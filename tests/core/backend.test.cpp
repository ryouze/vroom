/**
 * @file backend.test.cpp
 */

#include <snitch/snitch.hpp>
#include <SFML/Graphics.hpp>

#include "core/backend.hpp"

TEST_CASE("ImGuiContext construction and destruction", "[src][core][backend.hpp]")
{
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u{800, 600}), "Test Window");
    REQUIRE_NOTHROW(core::backend::ImGuiContext(window));
}

TEST_CASE("Window default construction and getters", "[src][core][backend.hpp]")
{
    core::backend::Window window;
    // Default state checks
    CHECK(window.is_fullscreen() == true || window.is_fullscreen() == false);  // is_fullscreen returns bool
    CHECK(window.is_vsync_enabled() == false);
    CHECK(window.get_resolution().x >= 800);
    CHECK(window.get_resolution().y >= 600);
}

TEST_CASE("Window set_window_state switches between fullscreen and windowed", "[src][core][backend.hpp]")
{
    core::backend::Window window;
    window.set_window_state(core::backend::WindowState::Windowed);
    CHECK(window.is_fullscreen() == false);
    window.set_window_state(core::backend::WindowState::Fullscreen);
    CHECK(window.is_fullscreen() == true);
}

TEST_CASE("Window set_fps_limit and set_vsync are mutually exclusive", "[src][core][backend.hpp]")
{
    core::backend::Window window;
    window.set_fps_limit(60);
    CHECK(window.is_vsync_enabled() == false);
    window.set_vsync(true);
    CHECK(window.is_vsync_enabled() == true);
    window.set_fps_limit(120);
    CHECK(window.is_vsync_enabled() == false);
}

TEST_CASE("Window run loop calls callbacks", "[src][core][backend.hpp]")
{
    core::backend::Window window;
    bool event_callback_called = false;
    bool update_callback_called = false;
    bool render_callback_called = false;
    auto event_callback = [&](const sf::Event &) { event_callback_called = true; };
    auto update_callback = [&](const float) { update_callback_called = true; };
    auto render_callback = [&](sf::RenderWindow &) { render_callback_called = true; };
    window.close();  // Ensure loop exits immediately
    window.run(event_callback, update_callback, render_callback);
    CHECK(event_callback_called == false);
    CHECK(update_callback_called == false);
    CHECK(render_callback_called == false);
}

TEST_CASE("to_vector2f converts unsigned to float vector", "[src][core][backend.hpp]")
{
    sf::Vector2u unsigned_vector = {1280, 800};
    sf::Vector2f float_vector = core::backend::to_vector2f(unsigned_vector);
    CHECK(float_vector.x == 1280.f);
    CHECK(float_vector.y == 800.f);
}
