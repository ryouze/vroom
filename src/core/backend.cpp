/**
 * @file backend.cpp
 */

#include <algorithm>  // for std::min
#include <cstddef>    // for std::size_t
#include <format>     // for std::format
#include <string>     // for std::string

#include <SFML/Graphics.hpp>
#include <spdlog/spdlog.h>

#include "backend.hpp"
#include "generated.hpp"
#include "settings.hpp"

namespace core::backend {

Window::Window()
{
    this->create();
}

void Window::recreate()
{
    SPDLOG_DEBUG("Recreating window with current settings");
    this->create();
    SPDLOG_DEBUG("Window recreated successfully");
}

void Window::run(const event_callback_type &on_event,
                 const update_callback_type &on_update,
                 const render_callback_type &on_render)
{
    SPDLOG_INFO("Starting main window loop!");
    sf::Clock clock;
    while (this->window_.isOpen()) {
        // Allow user of this call to explicitly handle events themselves
        this->window_.handleEvents(on_event);
        // Prevent extreme dt by clamping to 0.1 seconds
        constexpr float dt_max = 0.1f;
        const float dt = std::min(clock.restart().asSeconds(), dt_max);
        on_update(dt);
        on_render(this->window_);
    }
    SPDLOG_INFO("Main window loop ended!");
}

void Window::create()
{
    // Create context settings with the current anti-aliasing level
    const sf::ContextSettings settings{.antiAliasingLevel = settings::constants::anti_aliasing_values[settings::current::anti_aliasing_idx]};
    SPDLOG_DEBUG("Created context settings with '{}' anti-aliasing level", settings.antiAliasingLevel);

    // Create the window title based on the project name and version
    const std::string window_title = std::format("{} ({})", generated::PROJECT_NAME, generated::PROJECT_VERSION);
    SPDLOG_DEBUG("Created '{}' window title", window_title);

    // Get the window state based on current settings
    const sf::State state = settings::current::fullscreen ? sf::State::Fullscreen : sf::State::Windowed;

    // Get the video mode (resolution) based on current settings
    sf::VideoMode mode;
    if (settings::current::fullscreen) {
        // If fullscreen, we need to determine the video mode based on current settings (the "config.toml" file)
        SPDLOG_DEBUG("Current mode is fullscreen, determining video mode based on current settings...");

        // Run a sanity check on the resolution indices and available modes
        if (!this->available_fullscreen_modes.empty() &&                                            // Non-empty list (probably impossible)
            settings::current::mode_idx >= 0 &&                                                     // Config index is non-negative (different monitor?)
            settings::current::mode_idx < static_cast<int>(this->available_fullscreen_modes.size()  // Config index is within bounds (different monitor?)
                                                           )) {
            // If everything is valid, use the selected mode from "config.toml"
            mode = this->available_fullscreen_modes[static_cast<std::size_t>(settings::current::mode_idx)];
            SPDLOG_DEBUG("Current settings are valid, set video mode to '{}x{}' (current index: '{}')", mode.size.x, mode.size.y, settings::current::mode_idx);
        }

        else {
            // Otherwise, use the desktop mode as fallback and reset the config index to best available mode so it won't cause issues next time
            mode = sf::VideoMode::getDesktopMode();
            settings::current::mode_idx = 0;
            SPDLOG_WARN("Current settings are invalid, falling back to desktop mode '{}x{}' and resetting current index to '0'", mode.size.x, mode.size.y);
        }
    }
    else {
        // If windowed, use the default windowed resolution from settings
        // We don't store user preferences for windowed mode, because fullscreen is the primary way of running the game
        mode = sf::VideoMode{sf::Vector2u{settings::constants::windowed_width,
                                          settings::constants::windowed_height}};
        SPDLOG_DEBUG("Current mode is windowed, using default resolution '{}x{}'", mode.size.x, mode.size.y);
    }

    // If window is already open, close it, so the "create" method can recreate it
    if (this->window_.isOpen()) {
        SPDLOG_DEBUG("Window was already open, closing it so we can recreate it with new values");
        this->window_.close();
    }

    // Create the window with the determined video mode, title, state, and context settings
    this->window_.create(mode, window_title, state, settings);

    // Set minimum size (only relevant for windowed mode)
    this->window_.setMinimumSize(sf::Vector2u{settings::constants::windowed_min_width,
                                              settings::constants::windowed_min_height});

    // Set FPS/vsync settings
    if (settings::current::vsync) {
        // If vsync is enabled, disable FPS limit (0 = disabled)
        this->window_.setFramerateLimit(0u);
        // Then, enable vsync
        this->window_.setVerticalSyncEnabled(true);
        SPDLOG_DEBUG("Disabled FPS limit and enabled V-sync");
    }
    else {
        // If vsync is disabled, disable vsync
        this->window_.setVerticalSyncEnabled(false);
        // Then, get the FPS limit and enable it
        const unsigned fps_limit = settings::constants::fps_values[settings::current::fps_idx];

        if (fps_limit == 0) {
            SPDLOG_WARN("FPS limit is set to '0', which means no limit!");
        }

        this->window_.setFramerateLimit(fps_limit);
        SPDLOG_DEBUG("Enabled '{}' FPS limit and disabled V-sync", fps_limit);
    }

    // Log the successful creation of the window
    SPDLOG_DEBUG("Window created successfully with mode '{}x{}', title '{}', state '{}', and context settings (anti-aliasing level: {})", mode.size.x, mode.size.y, window_title, state == sf::State::Fullscreen ? "fullscreen" : "windowed", settings.antiAliasingLevel);
}

sf::Vector2f to_vector2f(const sf::Vector2u &vector)
{
    return {static_cast<float>(vector.x), static_cast<float>(vector.y)};
}

}  // namespace core::backend
