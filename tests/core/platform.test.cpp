/**
 * @file platform.test.cpp
 */

#include <snitch/snitch.hpp>

#if defined(_WIN32)
#include "core/platform/windows.hpp"
#else  // Assumption: if not Windows, then POSIX
#include "core/platform/posix.hpp"
#endif

#if defined(_WIN32)
TEST_CASE("Windows local appdata directory returns a valid path", "[src][core][platform.hpp]")
{
    const std::filesystem::path appdata_path = core::platform::windows::get_local_appdata_directory();
    CHECK(!appdata_path.empty());
    CHECK(appdata_path.string().find(":\\") != std::string::npos);  // Basic check for Windows path format
}
#else  // Assumption: if not Windows, then POSIX
TEST_CASE("POSIX home directory returns a valid path", "[src][core][platform.hpp]")
{
    const std::filesystem::path home_path = core::platform::posix::get_home_directory();
    CHECK(!home_path.empty());
    CHECK(home_path.string()[0] == '/');  // Basic check for POSIX path format
}
#endif
