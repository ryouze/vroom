/**
 * @file posix.cpp
 */

#if !defined(_WIN32)  // Assumption: if not Windows, then POSIX

#include <cerrno>      // for ENOENT
#include <cstddef>     // for std::size_t
#include <cstdlib>     // for std::getenv
#include <cstring>     // for std::strerror
#include <filesystem>  // for std::filesystem
#include <format>      // for std::format
#include <stdexcept>   // for std::runtime_error
#include <vector>      // for std::vector

#include <pwd.h>     // for passwd, getpwuid_r
#include <unistd.h>  // for getuid, sysconf, _SC_GETPW_R_SIZE_MAX, uid_t

#include <spdlog/spdlog.h>

#include "posix.hpp"

namespace core::platform::posix {

std::filesystem::path get_home_directory()
{
    SPDLOG_DEBUG("Retrieving home directory from environment variable '$HOME'...");

    if (const char *const home_env = std::getenv("HOME"); home_env && *home_env != '\0') [[likely]] {
        SPDLOG_DEBUG("Home directory successfully retrieved as '{}', returning it!", home_env);
        return std::filesystem::path(home_env);
    }

    // Note: The env variable works in most cases, whereas the passwd code below is NOT well tested, but should probably work

    SPDLOG_WARN("Failed to retrieve home directory from environment variable, falling back to passwd database...");

    // Determine the buffer size for getpwuid_r
    constexpr std::size_t fallback_buffer_size = 1024;
    const long sysconf_size = ::sysconf(_SC_GETPW_R_SIZE_MAX);
    const std::size_t buffer_size = sysconf_size > 0 ? static_cast<std::size_t>(sysconf_size) : fallback_buffer_size;
    SPDLOG_DEBUG("Calculated buffer size bytes='{}'!", buffer_size);

    // Allocate a buffer for the passwd structure
    std::vector<char> buffer(buffer_size);
    SPDLOG_DEBUG("Allocated buffer bytes='{}'!", buffer.size());

    // Initialize the passwd structure
    struct passwd pwd{};
    struct passwd *result = nullptr;
    const uid_t uid = ::getuid();

    // Call getpwuid_r to retrieve the passwd structure
    SPDLOG_DEBUG("Calling getpwuid_r...");
    const int ret = ::getpwuid_r(uid, &pwd, buffer.data(), buffer.size(), &result);
    if (ret == 0 && result && pwd.pw_dir && *pwd.pw_dir != '\0') [[likely]] {
        SPDLOG_DEBUG("Home directory successfully retrieved as '{}', returning it!", pwd.pw_dir);
        return std::filesystem::path(pwd.pw_dir);
    }

    // Use ret if it is non‑zero; otherwise, no passwd entry (ENOENT)
    throw std::runtime_error(std::format("Failed to get the home directory on a POSIX system: {}", std::strerror(ret != 0 ? ret : ENOENT)));
}

}  // namespace core::platform::posix

#endif
