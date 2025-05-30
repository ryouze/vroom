/**
 * @file posix.hpp
 *
 * @brief POSIX (macOS, Linux) platform-specific functions.
 */

#pragma once

#if !defined(_WIN32) // Assumption: if not Windows, then POSIX

#include <filesystem>  // for std::filesystem

namespace core::platform::posix {

/**
 * @brief Get the path to the home directory on a POSIX system (macOS, Linux).
 *
 * @return Path to the home directory (e.g., "/home/user" on Linux, "/Users/username" on macOS).
 *
 * @throws std::runtime_error if failed to retrieve the path.
 */
[[nodiscard]] std::filesystem::path get_home_directory();

}  // namespace core::platform::posix

#endif
