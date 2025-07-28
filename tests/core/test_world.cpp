/**
 * @file test_world.cpp
 */

#include <snitch/snitch.hpp>

#include "core/world.hpp"

TEST_CASE("TrackConfig equality operator works for identical configs", "[core][world]")
{
    const core::world::TrackConfig config1;
    const core::world::TrackConfig config2;
    REQUIRE(config1 == config2);
}

TEST_CASE("TrackConfig equality operator detects different configs", "[core][world]")
{
    const core::world::TrackConfig config1;
    core::world::TrackConfig config2;
    config2.horizontal_count = 8;
    CHECK_FALSE(config1 == config2);
}
