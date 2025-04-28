/**
 * @file windows.hpp
 *
 * @brief Microsoft Windows platform-specific functions.
 */

#pragma once

#if defined(_WIN32)

#include <filesystem>  // for std::filesystem

namespace core::platform::windows {

/**
 * @brief Get the path to the local AppData directory on Windows.
 *
 * @return Path to the local AppData directory (e.g., "C:/Users/<username>/AppData/Local").
 *
 * @throws std::runtime_error if failed to retrieve the path.
 */
[[nodiscard]] std::filesystem::path get_local_appdata_directory();

}  // namespace core::platform::windows

#endif
