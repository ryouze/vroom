/**
 * @file ui.test.cpp
 */

#include <SFML/Graphics.hpp>
#include <snitch/snitch.hpp>

#include "core/ui.hpp"

TEST_CASE("Corner enum values", "[src][core][ui.hpp]")
{
    REQUIRE(static_cast<int>(core::ui::Corner::TopLeft) == 0);
    REQUIRE(static_cast<int>(core::ui::Corner::TopRight) == 1);
    REQUIRE(static_cast<int>(core::ui::Corner::BottomLeft) == 2);
    REQUIRE(static_cast<int>(core::ui::Corner::BottomRight) == 3);
}

TEST_CASE("IWidget default enabled", "[src][core][ui.hpp]")
{
    struct DummyWidget : public core::ui::IWidget {
        void update_and_draw() {}
    } widget;
    REQUIRE(widget.enabled == true);
}

TEST_CASE("FpsCounter construction and update_and_draw does not throw", "[src][core][ui.hpp]")
{
    sf::RenderTexture window({1280, 720});
    core::ui::FpsCounter fps(window, core::ui::Corner::TopLeft);
    REQUIRE(fps.enabled == true);
    REQUIRE_NOTHROW(fps.update_and_draw(0.016f));
}

TEST_CASE("Speedometer construction and update_and_draw does not throw", "[src][core][ui.hpp]")
{
    sf::RenderTexture window({1280, 720});
    core::ui::Speedometer speedo(window, core::ui::Corner::BottomRight);
    REQUIRE(speedo.enabled == true);
    REQUIRE_NOTHROW(speedo.update_and_draw(123.4f));
}

TEST_CASE("Minimap construction and update_and_draw does not throw", "[src][core][ui.hpp]")
{
    sf::RenderTexture window({1280, 720});
    sf::Color bg = sf::Color::Black;
    core::ui::Minimap::GameEntitiesDrawer drawer = [](sf::RenderTarget &) {};
    core::ui::Minimap minimap(window, bg, drawer, core::ui::Corner::BottomLeft);
    REQUIRE(minimap.enabled == true);
    REQUIRE_NOTHROW(minimap.update_and_draw(0.016f, sf::Vector2f{0.f, 0.f}));
}

TEST_CASE("Minimap set/get resolution", "[src][core][ui.hpp]")
{
    sf::RenderTexture window({1280, 720});
    sf::Color bg = sf::Color::Black;
    core::ui::Minimap::GameEntitiesDrawer drawer = [](sf::RenderTarget &) {};
    core::ui::Minimap minimap(window, bg, drawer, core::ui::Corner::BottomLeft);
    sf::Vector2u new_res = {512u, 512u};
    REQUIRE_NOTHROW(minimap.set_resolution(new_res));
    REQUIRE(minimap.get_resolution() == new_res);
}

TEST_CASE("LeaderboardEntry struct default values", "[src][core][ui.hpp]")
{
    core::ui::LeaderboardEntry entry;
    REQUIRE(entry.car_name.empty());
    REQUIRE(entry.drift_score == 0.0f);
    REQUIRE(entry.is_player == false);
}

TEST_CASE("Leaderboard construction and update_and_draw does not throw", "[src][core][ui.hpp]")
{
    sf::RenderTexture window({1280, 720});
    core::ui::Leaderboard leaderboard(window, core::ui::Corner::TopRight);
    REQUIRE(leaderboard.enabled == true);
    std::vector<core::ui::LeaderboardEntry> entries = {
        {"You", 123.4f, true},
        {"Blue", 100.0f, false},
        {"Red", 50.0f, false}};
    REQUIRE_NOTHROW(leaderboard.update_and_draw(entries));
}
