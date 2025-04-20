/**
 * @file io.cpp
 */

#include <cstdlib>     // for std::getenv
#include <exception>   // for std::exception
#include <filesystem>  // for std::filesystem
#include <format>      // for std::format
#include <stdexcept>   // for std::runtime_error
#include <string>      // for std::string

#include <spdlog/spdlog.h>

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

}  // namespace core::io
