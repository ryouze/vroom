/**
 * @file io.hpp
 *
 * @brief Input/output utilities.
 */

#pragma once

#include <filesystem>  // for std::filesystem
#include <string>      // for std::string

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

// Load and save configuration files to disk
class Config {
  public:
    explicit Config(const std::filesystem::path &path = get_application_storage_location(generated::PROJECT_NAME))
        : path_(path)
    {
        // SDPLOG not imported yet
        // SPDLOG_DEBUG("Config path set to '{}'", path_.string());

        // TODO: Create the directory if it doesn't exist, then store and load settings (e.g., FPS limit, minimap on/off, etc.)
        // const std::filesystem::path appdata_path = core::io::get_application_storage_location(generated::PROJECT_NAME);
    }

  private:
    const std::filesystem::path path_;
};

}  // namespace core::io
