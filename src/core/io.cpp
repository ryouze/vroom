/**
 * @file io.cpp
 */

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

Config::Config(const std::string &filename)
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
            this->show_fps_counter = tbl["show_fps_counter"].value_or(this->show_fps_counter);
            this->show_minimap = tbl["show_minimap"].value_or(this->show_minimap);
            this->show_speedometer = tbl["show_speedometer"].value_or(this->show_speedometer);
            this->show_leaderboard = tbl["show_leaderboard"].value_or(this->show_leaderboard);
            this->vsync_enabled = tbl["vsync_enabled"].value_or(this->vsync_enabled);
            this->fullscreen_enabled = tbl["fullscreen_enabled"].value_or(this->fullscreen_enabled);
            this->resolution_index = tbl["resolution_index"].value_or(this->resolution_index);
            this->fps_limit_index = tbl["fps_limit_index"].value_or(this->fps_limit_index);
            this->minimap_refresh_interval = tbl["minimap_refresh_interval"].value_or(this->minimap_refresh_interval);
            this->minimap_resolution_index = tbl["minimap_resolution_index"].value_or(this->minimap_resolution_index);
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

Config::~Config() noexcept
{
    try {
        this->save();
    }
    catch (...) {
    }
}

void Config::save() const noexcept
{
    //     SPDLOG_DEBUG("Now saving config to '{}'", this->path_.string());

    toml::table tbl;
    tbl.insert_or_assign("show_fps_counter", this->show_fps_counter);
    tbl.insert_or_assign("show_minimap", this->show_minimap);
    tbl.insert_or_assign("show_speedometer", this->show_speedometer);
    tbl.insert_or_assign("show_leaderboard", this->show_leaderboard);
    tbl.insert_or_assign("vsync_enabled", this->vsync_enabled);
    tbl.insert_or_assign("fullscreen_enabled", this->fullscreen_enabled);
    tbl.insert_or_assign("resolution_index", this->resolution_index);
    tbl.insert_or_assign("fps_limit_index", this->fps_limit_index);
    tbl.insert_or_assign("minimap_refresh_interval", this->minimap_refresh_interval);
    tbl.insert_or_assign("minimap_resolution_index", this->minimap_resolution_index);

    std::ofstream ofs(this->path_, std::ios::trunc);
    if (!ofs) {
        SPDLOG_ERROR("Cannot open config file for writing: '{}'", this->path_.string());
        return;
    }
    ofs << tbl;
    SPDLOG_DEBUG("Config was sucessfully saved to '{}'", this->path_.string());
}

}  // namespace core::io
