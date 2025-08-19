/**
 * @file colors.test.cpp
 */

#include <snitch/snitch.hpp>

#include "core/colors.hpp"

TEST_CASE("Color constants have valid RGB values", "[src][core][colors.hpp]")
{
    // Test that all colors have valid RGB values (0-255)
    CHECK(core::colors::window.menu.r <= 255);
    CHECK(core::colors::window.menu.g <= 255);
    CHECK(core::colors::window.menu.b <= 255);
    CHECK(core::colors::window.menu.a <= 255);

    CHECK(core::colors::window.game.r <= 255);
    CHECK(core::colors::window.game.g <= 255);
    CHECK(core::colors::window.game.b <= 255);
    CHECK(core::colors::window.game.a <= 255);

    CHECK(core::colors::window.settings.r <= 255);
    CHECK(core::colors::window.settings.g <= 255);
    CHECK(core::colors::window.settings.b <= 255);
    CHECK(core::colors::window.settings.a <= 255);
}
