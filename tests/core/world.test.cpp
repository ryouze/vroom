/**
 * @file world.test.cpp
 */

#include <snitch/snitch.hpp>

#include "core/world.hpp"

TEST_CASE("TrackConfig equality operator works for identical configs", "[src][core][world.hpp]")
{
    const core::world::TrackConfig config1;
    const core::world::TrackConfig config2;

    REQUIRE(config1 == config2);
}

TEST_CASE("TrackConfig equality operator detects different configs", "[src][core][world.hpp]")
{
    const core::world::TrackConfig config1;
    core::world::TrackConfig config2;

    constexpr int non_default_horizontal_count = 8;
    config2.horizontal_count = non_default_horizontal_count;

    CHECK_FALSE(config1 == config2);
}
