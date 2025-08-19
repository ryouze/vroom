/**
 * @file io.test.cpp
 */

#include <filesystem>  // for std::filesystem
#include <string>      // for std::string

#include <snitch/snitch.hpp>

#include "core/io.hpp"

TEST_CASE("get_application_storage_location returns non-empty path", "[src][core][io.hpp]")
{
    // Test with a basic application name
    const std::filesystem::path path = core::io::get_application_storage_location("TestApp");
    CHECK(!path.empty());
    CHECK(!path.string().empty());
}

TEST_CASE("get_application_storage_location handles different app names", "[src][core][io.hpp]")
{
    // Test that different app names produce different paths
    const std::filesystem::path path1 = core::io::get_application_storage_location("App1");
    const std::filesystem::path path2 = core::io::get_application_storage_location("App2");

    CHECK(path1 != path2);
    CHECK(path1.string().find("App1") != std::string::npos);
    CHECK(path2.string().find("App2") != std::string::npos);
}
