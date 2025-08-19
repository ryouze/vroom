/**
 * @file io.cpp
 */

#include <algorithm>   // for std::clamp, std::max
#include <array>       // for std::size
#include <cstdlib>     // for std::getenv
#include <exception>   // for std::exception
#include <filesystem>  // for std::filesystem
#include <format>      // for std::format
#include <fstream>     // for std::ofstream
#include <ios>         // for std::ios
#include <stdexcept>   // for std::runtime_error
#include <string>      // for std::string

#include <spdlog/spdlog.h>
#include <toml++/toml.hpp>

#if defined(_WIN32)
#include "platform/windows.hpp"
#else  // Assumption: if not Windows, then POSIX
#include "platform/posix.hpp"
#endif
#include "io.hpp"
#include "settings.hpp"

namespace core::io {

std::filesystem::path get_application_storage_location(const std::string &application_name)
{
    // These platform-specific abstractions can throw; catch-rethrow them here to add context of getting the app storage directory
    try {
        std::filesystem::path base_dir;
#if defined(__APPLE__)
        SPDLOG_DEBUG("Acquiring AppData path for macOS by calling an abstracted function...");
        // ~/Library/Application Support
        base_dir = platform::posix::get_home_directory() / "Library" / "Application Support";
#elif defined(_WIN32)
        SPDLOG_DEBUG("Acquiring AppData path for Windows by calling an abstracted function...");
        // C:/Users/<username>/AppData/Local
        base_dir = platform::windows::get_local_appdata_directory();
        // // Note: This only supports ASCII paths, but I'd rather have broken paths than deal with the Windows API
        // const char *local_appdata_env = std::getenv("LOCALAPPDATA");
        // if (local_appdata_env == nullptr || *local_appdata_env == '\0') {
        //     throw std::runtime_error("LOCALAPPDATA environment variable not set");
        // }
#else
        SPDLOG_DEBUG("Acquiring AppData path for Linux/POSIX by calling an abstracted function...");
        // XDG_DATA_HOME or ~/.local/share
        const char *const xdg_data_home = std::getenv("XDG_DATA_HOME");
        base_dir = (xdg_data_home && *xdg_data_home)
                       ? std::filesystem::path(xdg_data_home)
                       : platform::posix::get_home_directory() / ".local" / "share";
#endif

        SPDLOG_DEBUG("Constructing the AppData path with base directory '{}' and application name '{}'...", base_dir.string(), application_name);

        // Append the application name to the base directory, then normalize the path
        const std::filesystem::path result = std::filesystem::absolute(base_dir / application_name).lexically_normal();

        SPDLOG_DEBUG("AppData path created successfully as '{}', returning it!", result.string());
        return result;
    }
    catch (const std::exception &e) {
        // Re-throw any other exceptions as std::runtime_error
        throw std::runtime_error(std::format("Failed to get path to the app storage directory: {}", e.what()));
    }
}

ConfigContext::ConfigContext(const std::string &filename)
    : path_(get_application_storage_location(generated::PROJECT_NAME) / filename)
{
    SPDLOG_DEBUG("Created config path: '{}'", this->path_.string());

    try {
        // Ensure directory exists before doing anything
        if (!std::filesystem::exists(this->path_.parent_path())) {
            std::filesystem::create_directories(this->path_.parent_path());
            SPDLOG_DEBUG("Created missing config directory: '{}'", this->path_.parent_path().string());
        }
        // else {
        //     SPDLOG_DEBUG("Config directory already exists, no need to create it");
        // }

        // If the file exists, load it, otherwise create it with defaults
        if (std::filesystem::exists(this->path_)) {
            const toml::table tbl = toml::parse_file(this->path_.string());
            settings::current::fullscreen = tbl["fullscreen"].value_or(settings::current::fullscreen);
            settings::current::vsync = tbl["vsync"].value_or(settings::current::vsync);

            // Clamp fps_idx to valid range [0, size-1] to prevent out-of-bounds access
            const int loaded_fps_idx = tbl["fps_idx"].value_or(settings::current::fps_idx);
            settings::current::fps_idx = std::clamp(loaded_fps_idx, 0, static_cast<int>(std::size(settings::constants::fps_values)) - 1);

            // Clamp mode_idx to be non-negative (upper bound is checked in backend.cpp)
            const int loaded_mode_idx = tbl["mode_idx"].value_or(settings::current::mode_idx);
            settings::current::mode_idx = std::max(loaded_mode_idx, 0);

            // Clamp anti_aliasing_idx to valid range [0, size-1] to prevent out-of-bounds access
            const int loaded_anti_aliasing_idx = tbl["anti_aliasing_idx"].value_or(settings::current::anti_aliasing_idx);
            settings::current::anti_aliasing_idx = std::clamp(loaded_anti_aliasing_idx, 0, static_cast<int>(std::size(settings::constants::anti_aliasing_values)) - 1);

            settings::current::prefer_gamepad = tbl["prefer_gamepad"].value_or(settings::current::prefer_gamepad);
            settings::current::gamepad_steering_axis = tbl["gamepad_steering_axis"].value_or(settings::current::gamepad_steering_axis);
            settings::current::gamepad_gas_axis = tbl["gamepad_gas_axis"].value_or(settings::current::gamepad_gas_axis);
            settings::current::gamepad_brake_axis = tbl["gamepad_brake_axis"].value_or(settings::current::gamepad_brake_axis);
            settings::current::gamepad_handbrake_button = tbl["gamepad_handbrake_button"].value_or(settings::current::gamepad_handbrake_button);
            settings::current::gamepad_invert_steering = tbl["gamepad_invert_steering"].value_or(settings::current::gamepad_invert_steering);
            settings::current::gamepad_invert_gas = tbl["gamepad_invert_gas"].value_or(settings::current::gamepad_invert_gas);
            settings::current::gamepad_invert_brake = tbl["gamepad_invert_brake"].value_or(settings::current::gamepad_invert_brake);

            settings::current::engine_volume = tbl["engine_volume"].value_or(settings::current::engine_volume);
            settings::current::tire_screech_volume = tbl["tire_screech_volume"].value_or(settings::current::tire_screech_volume);
            settings::current::wall_hit_volume = tbl["wall_hit_volume"].value_or(settings::current::wall_hit_volume);
            settings::current::ui_volume = tbl["ui_volume"].value_or(settings::current::ui_volume);

            SPDLOG_DEBUG("Config was loaded successfully!");
        }
        else {
            this->save();
            SPDLOG_DEBUG("Config file was missing, created with default values!");
        }
    }
    catch (const toml::parse_error &err) {
        SPDLOG_ERROR("Failed to parse TOML file '{}': {}", this->path_.string(), err.description());
        this->save();
    }
    catch (const std::exception &e) {
        SPDLOG_ERROR("Failed to loa TOML file '{}': {}", this->path_.string(), e.what());
        this->save();
    }
}

ConfigContext::~ConfigContext() noexcept
{
    try {
        this->save();
    }
    catch (...) {
    }
}

void ConfigContext::save() const noexcept
{
    //     SPDLOG_DEBUG("Now saving config to '{}'", this->path_.string());

    toml::table tbl;
    tbl.insert_or_assign("fullscreen", settings::current::fullscreen);
    tbl.insert_or_assign("vsync", settings::current::vsync);
    tbl.insert_or_assign("fps_idx", settings::current::fps_idx);
    tbl.insert_or_assign("mode_idx", settings::current::mode_idx);
    tbl.insert_or_assign("anti_aliasing_idx", settings::current::anti_aliasing_idx);
    tbl.insert_or_assign("prefer_gamepad", settings::current::prefer_gamepad);
    tbl.insert_or_assign("gamepad_steering_axis", settings::current::gamepad_steering_axis);
    tbl.insert_or_assign("gamepad_gas_axis", settings::current::gamepad_gas_axis);
    tbl.insert_or_assign("gamepad_brake_axis", settings::current::gamepad_brake_axis);
    tbl.insert_or_assign("gamepad_handbrake_button", settings::current::gamepad_handbrake_button);
    tbl.insert_or_assign("gamepad_invert_steering", settings::current::gamepad_invert_steering);
    tbl.insert_or_assign("gamepad_invert_gas", settings::current::gamepad_invert_gas);
    tbl.insert_or_assign("gamepad_invert_brake", settings::current::gamepad_invert_brake);
    tbl.insert_or_assign("engine_volume", settings::current::engine_volume);
    tbl.insert_or_assign("tire_screech_volume", settings::current::tire_screech_volume);
    tbl.insert_or_assign("wall_hit_volume", settings::current::wall_hit_volume);
    tbl.insert_or_assign("ui_volume", settings::current::ui_volume);

    std::ofstream ofs(this->path_, std::ios::trunc);
    if (!ofs) {
        SPDLOG_ERROR("Cannot open config file for writing: '{}'", this->path_.string());
        return;
    }
    ofs << tbl;
    SPDLOG_DEBUG("Config was sucessfully saved to '{}'", this->path_.string());
}

}  // namespace core::io
