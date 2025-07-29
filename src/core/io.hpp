/**
 * @file io.hpp
 *
 * @brief Input/output utilities.
 */

#pragma once

#include <filesystem>  // for std::filesystem
#include <string>  // for std::string

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
    explicit Config(const std::string &filename = "config.toml");

    /**
     * @brief Destroy the Config object.
     *
     * On destruction, save the current configuration state to the TOML file.
     */
    ~Config() noexcept;

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
    void save() const noexcept;

    /**
     * @brief Path to the configuration TOML file.
     */
    const std::filesystem::path path_;
};

}  // namespace core::io
