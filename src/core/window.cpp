/**
 * @file window.cpp
 */

#include <algorithm>  // for std::min
#include <stdexcept>  // for std::runtime_error

#include <SFML/Graphics.hpp>
#include <spdlog/spdlog.h>

#include "generated.hpp"
#include "settings.hpp"
#include "window.hpp"

namespace core::window {

Window::Window()
{
    SPDLOG_DEBUG("Initializing window with current settings: fullscreen='{}', vsync='{}', fps_idx='{}', resolution_idx='{}'",
                 settings::current::fullscreen, settings::current::vsync, settings::current::fps_idx, settings::current::resolution_idx);

    // Initialize window with current settings
    this->initialize_with_current_settings();
}

void Window::initialize_with_current_settings()
{
    SPDLOG_DEBUG("Initializing window with current settings");

    // Get the appropriate video mode for current settings
    const sf::VideoMode mode = this->get_video_mode_for_current_settings();
    const sf::State state = settings::current::fullscreen ? sf::State::Fullscreen : sf::State::Windowed;

    // Create or recreate the window
    if (this->window_.isOpen()) {
        SPDLOG_DEBUG("Recreating existing window with mode '{}x{}' in '{}' state",
                     mode.size.x, mode.size.y, settings::current::fullscreen ? "fullscreen" : "windowed");
        this->window_.close();
    }
    else {
        SPDLOG_DEBUG("Creating new window with mode '{}x{}' in '{}' state",
                     mode.size.x, mode.size.y, settings::current::fullscreen ? "fullscreen" : "windowed");
    }

    this->create_window(mode, state);
    SPDLOG_DEBUG("Window initialized successfully");
}

sf::VideoMode Window::get_video_mode_for_current_settings() const
{
    if (settings::current::fullscreen) {
        const auto available_modes = get_available_modes();
        if (!available_modes.empty() && settings::current::resolution_idx >= 0 &&
            settings::current::resolution_idx < static_cast<int>(available_modes.size())) {
            const sf::VideoMode selected_mode = available_modes[static_cast<std::size_t>(settings::current::resolution_idx)];
            SPDLOG_DEBUG("Using fullscreen mode '{}x{}' from settings (index {})",
                         selected_mode.size.x, selected_mode.size.y, settings::current::resolution_idx);
            return selected_mode;
        }
        else {
            SPDLOG_DEBUG("Invalid resolution index '{}', falling back to desktop mode", settings::current::resolution_idx);
            return sf::VideoMode::getDesktopMode();
        }
    }
    else {
        const sf::Vector2u windowed_resolution = {settings::defaults::windowed_width, settings::defaults::windowed_height};
        SPDLOG_DEBUG("Using windowed mode '{}x{}'", windowed_resolution.x, windowed_resolution.y);
        return sf::VideoMode{windowed_resolution};
    }
}

std::vector<sf::VideoMode> Window::get_available_modes()
{
    auto modes = sf::VideoMode::getFullscreenModes();
    SPDLOG_DEBUG("Found '{}' available fullscreen modes", modes.size());
    return modes;
}

void Window::toggle_fullscreen()
{
    SPDLOG_DEBUG("Toggling fullscreen mode from '{}' to '{}'", 
                 settings::current::fullscreen ? "fullscreen" : "windowed", 
                 settings::current::fullscreen ? "windowed" : "fullscreen");

    // Toggle the fullscreen setting
    settings::current::fullscreen = !settings::current::fullscreen;

    // Apply the changes by reinitializing with current settings
    this->initialize_with_current_settings();

    SPDLOG_DEBUG("Fullscreen mode toggled successfully");
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

void Window::create_window(const sf::VideoMode &mode,
                           const sf::State state)
{
    SPDLOG_DEBUG("Creating window with video mode '{}x{}' in '{}' state", mode.size.x, mode.size.y, (state == sf::State::Fullscreen) ? "fullscreen" : "windowed");
    const sf::ContextSettings settings{.antiAliasingLevel = settings::defaults::anti_aliasing_level};
    const std::string window_title = std::format("{} ({})", generated::PROJECT_NAME, generated::PROJECT_VERSION);
    this->window_.create(mode, window_title, state, settings);
    this->window_.setMinimumSize(sf::Vector2u{settings::defaults::minimum_width, settings::defaults::minimum_height});
    this->apply_sync_settings();
    SPDLOG_DEBUG("Window created successfully with anti-aliasing level '{}'", settings::defaults::anti_aliasing_level);
}

void Window::apply_sync_settings()
{
    const unsigned frame_limit = settings::fps::values[static_cast<std::size_t>(settings::current::fps_idx)];
    SPDLOG_DEBUG("Applying sync settings with V-sync='{}' and frame_limit='{}'", settings::current::vsync, frame_limit);
    this->window_.setVerticalSyncEnabled(settings::current::vsync);
    this->window_.setFramerateLimit(settings::current::vsync ? 0u : frame_limit);
}

void Window::apply_current_settings()
{
    SPDLOG_DEBUG("Applying current settings from centralized configuration");

    // Simply reinitialize the window with current settings
    // This is much cleaner than trying to selectively apply changes
    this->initialize_with_current_settings();

    SPDLOG_DEBUG("Applied settings: fullscreen={}, vsync={}, fps_idx={} ({}), resolution_idx={}",
                 settings::current::fullscreen, settings::current::vsync, settings::current::fps_idx,
                 settings::fps::values[static_cast<std::size_t>(settings::current::fps_idx)], settings::current::resolution_idx);
}

sf::Vector2f to_vector2f(const sf::Vector2u &vector)
{
    return {static_cast<float>(vector.x), static_cast<float>(vector.y)};
}

}  // namespace core::window
