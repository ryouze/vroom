/**
 * @file io.hpp
 *
 * @brief Input/output utilities.
 */

#pragma once

#include <filesystem>  // for std::filesystem
#include <fstream>
#include <string>  // for std::string

#include <spdlog/spdlog.h>  // Remove later once moved to .cpp
#include <toml++/toml.hpp>

#include "generated.hpp"

namespace core::io {

/**
 * @brief Get the absolute path to the platform-specific application data directory.
 *
 * This function uses platform-specific APIs (e.g., POSIX, WinAPI) to resolve the path, but exposes a unified, platform-agnostic directory path. The directory itself is suitable or storing configuration files, logs, and other run-time application-specific resources.
 *
 * The platform-specific paths are as follows:
 * - macOS: "~/Library/Application Support/<application_name>"
 * - Linux: $XDG_DATA_HOME or "~/.local/share/<application_name>"
 * - Windows: "C:/Users/<username>/AppData/Local/<application_name>"
 *
 * @param application_name Name of the application (e.g., "MyApp"). This value is not validated; ensure it is a valid directory name.
 *
 * @return Absolute path to the AppData directory (e.g., "~/Library/Application Support/MyApp").
 *
 * @throws std::runtime_error if failed to retrieve the path.
 */
[[nodiscard]] std::filesystem::path get_application_storage_location(const std::string &application_name);

/**
 * @brief Class that abstracts TOML configuration management on disk.
 *
 * On construction, the class attempts to load a TOML configuration file from a platform-specific application data directory, creating default values if the file is missing.
 *
 * The file is saved automatically on destruction.
 */
class Config {
  public:
    /**
     * @brief Attempt to load a TOML configuration file from the platform-specific application data directory, creating default values if the file is missing.
     *
     * @param filename Name of the configuration file (default: "config.toml"). This filename will be appended to the platform-specific application data directory path (e.g., "~/Library/Application Support/MyApp/config.toml").
     */
    explicit Config(const std::string &filename = "config.toml")
        : path_(get_application_storage_location(generated::PROJECT_NAME) / filename)
    {
        SPDLOG_DEBUG("Created config path: '{}'", this->path_.string());

        try {
            // Ensure directory exists before doing anything
            if (!std::filesystem::exists(this->path_.parent_path())) {
                SPDLOG_DEBUG("Config directory doesn't exist, creating: '{}'", this->path_.parent_path().string());
                std::filesystem::create_directories(this->path_.parent_path());
            }
            else {
                SPDLOG_DEBUG("Config directory already exists, no need to create it");
            }

            // If the file exists, load it, otherwise create it with defaults
            if (std::filesystem::exists(this->path_)) {
                SPDLOG_DEBUG("Config file exists, reading it...");
                const toml::table tbl = toml::parse_file(this->path_.string());
                this->show_fps_counter_ = tbl["show_fps_counter"].value_or(this->show_fps_counter_);
                this->show_minimap_ = tbl["show_minimap"].value_or(this->show_minimap_);
                this->show_speedometer_ = tbl["show_speedometer"].value_or(this->show_speedometer_);
                this->show_leaderboard_ = tbl["show_leaderboard"].value_or(this->show_leaderboard_);
                this->vsync_enabled_ = tbl["vsync_enabled"].value_or(this->vsync_enabled_);
                this->fullscreen_enabled_ = tbl["fullscreen_enabled"].value_or(this->fullscreen_enabled_);
                SPDLOG_DEBUG("Config was loaded successfully!");
            }
            else {
                SPDLOG_DEBUG("Config file doesn't exist, writing defaults...");
                this->save();
                SPDLOG_DEBUG("Default values were written!");
            }
        }
        catch (const toml::parse_error &err) {
            SPDLOG_ERROR("Failed to parse TOML file '{}': {}", this->path_.string(), err.description());
            this->save();  // recover with defaults
        }
        catch (const std::exception &e) {
            SPDLOG_ERROR("Failed to loa TOML file '{}': {}", this->path_.string(), e.what());
            this->save();
        }
    }

    /**
     * @brief Destroy the Config object.
     *
     * On destruction, save the current configuration state to the TOML file.
     */
    ~Config() noexcept
    {
        try {
            this->save();
        }
        catch (...) {
        }
    }

    /**
     * @brief Show the FPS counter widget in the UI.
     */
    bool show_fps_counter_ = true;
    /**
     * @brief Show the minimap widget in the UI.
     */
    bool show_minimap_ = true;
    /**
     * @brief Show the speedometer widget in the UI.
     */
    bool show_speedometer_ = true;
    /**
     * @brief Show the leaderboard widget in the UI.
     */
    bool show_leaderboard_ = true;
    /**
     * @brief Enable vertical sync for the window.
     */
    bool vsync_enabled_ = true;
    /**
     * @brief Start the game in fullscreen mode.
     */
    bool fullscreen_enabled_ = false;

  private:
    /**
     * @brief Save the current configuration state to the TOML file.
     *
     * Failures are logged only, so callers can stay noexcept.
     */
    void save() const noexcept
    {
        SPDLOG_DEBUG("Now saving config to '{}'", this->path_.string());

        toml::table tbl;
        tbl.insert_or_assign("show_fps_counter", this->show_fps_counter_);
        tbl.insert_or_assign("show_minimap", this->show_minimap_);
        tbl.insert_or_assign("show_speedometer", this->show_speedometer_);
        tbl.insert_or_assign("show_leaderboard", this->show_leaderboard_);
        tbl.insert_or_assign("vsync_enabled", this->vsync_enabled_);
        tbl.insert_or_assign("fullscreen_enabled", this->fullscreen_enabled_);

        std::ofstream ofs(this->path_, std::ios::trunc);
        if (!ofs) {
            SPDLOG_ERROR("Cannot open config file for writing!");
            return;
        }
        ofs << tbl;
        SPDLOG_DEBUG("Config was saved successfully!");
    }

    /**
     * @brief Path to the configuration TOML file.
     */
    const std::filesystem::path path_;
};

}  // namespace core::io
