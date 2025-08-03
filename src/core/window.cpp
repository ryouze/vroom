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
    : fullscreen_(settings::current::fullscreen),
      vsync_enabled_(settings::current::vsync),
      frame_limit_(settings::fps::values[static_cast<std::size_t>(settings::current::fps_idx)]),
      windowed_resolution_(settings::defaults::windowed_width, settings::defaults::windowed_height)
{
    SPDLOG_DEBUG("Initializing window with current settings: fullscreen='{}', vsync='{}', fps_idx='{}', resolution_idx='{}'",
                 settings::current::fullscreen, settings::current::vsync, settings::current::fps_idx, settings::current::resolution_idx);

    // Initialize window with current settings
    this->initialize_with_current_settings();
}

void Window::initialize_with_current_settings()
{
    SPDLOG_DEBUG("Initializing window with current settings");

    // Update internal state to match current settings
    this->fullscreen_ = settings::current::fullscreen;
    this->vsync_enabled_ = settings::current::vsync;
    this->frame_limit_ = settings::fps::values[static_cast<std::size_t>(settings::current::fps_idx)];

    // Get the appropriate video mode for current settings
    const sf::VideoMode mode = this->get_video_mode_for_current_settings();
    const sf::State state = this->fullscreen_ ? sf::State::Fullscreen : sf::State::Windowed;

    // Create or recreate the window
    if (this->window_.isOpen()) {
        SPDLOG_DEBUG("Recreating existing window with mode '{}x{}' in '{}' state",
                     mode.size.x, mode.size.y, this->fullscreen_ ? "fullscreen" : "windowed");
        this->window_.close();
    }
    else {
        SPDLOG_DEBUG("Creating new window with mode '{}x{}' in '{}' state",
                     mode.size.x, mode.size.y, this->fullscreen_ ? "fullscreen" : "windowed");
    }

    this->create_window(mode, state);
    SPDLOG_DEBUG("Window initialized successfully");
}

sf::VideoMode Window::get_video_mode_for_current_settings() const
{
    if (this->fullscreen_) {
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
        SPDLOG_DEBUG("Using windowed mode '{}x{}'", this->windowed_resolution_.x, this->windowed_resolution_.y);
        return sf::VideoMode{this->windowed_resolution_};
    }
}

std::vector<sf::VideoMode> Window::get_available_modes()
{
    auto modes = sf::VideoMode::getFullscreenModes();
    SPDLOG_DEBUG("Found '{}' available fullscreen modes", modes.size());
    return modes;
}

void Window::set_window_state(const WindowState state,
                              const sf::VideoMode &mode)
{
    const bool target_fullscreen = (state == WindowState::Fullscreen);

    SPDLOG_DEBUG("Setting window state to '{}' with mode '{}x{}'",
                 target_fullscreen ? "fullscreen" : "windowed", mode.size.x, mode.size.y);

    // Update settings to match the requested change
    settings::current::fullscreen = target_fullscreen;

    // If switching to fullscreen with a specific mode, find and set the resolution index
    if (target_fullscreen) {
        const auto available_modes = get_available_modes();
        for (int i = 0; i < static_cast<int>(available_modes.size()); ++i) {
            if (available_modes[static_cast<std::size_t>(i)].size == mode.size) {
                settings::current::resolution_idx = i;
                SPDLOG_DEBUG("Set resolution index to '{}' for mode '{}x{}'", i, mode.size.x, mode.size.y);
                break;
            }
        }
    }

    // Apply the changes by reinitializing with current settings
    this->initialize_with_current_settings();

    SPDLOG_DEBUG("Window state changed successfully");
}

void Window::set_fps_limit(const unsigned fps_limit)
{
    SPDLOG_DEBUG("Setting FPS limit to '{}' and disabling V-sync", fps_limit);
    // Disallow fps limit when vsync is on
    this->vsync_enabled_ = false;
    this->frame_limit_ = fps_limit;
    this->apply_sync_settings();
}

void Window::set_vsync(const bool enable)
{
    SPDLOG_DEBUG("Setting V-sync to '{}' ({})", enable ? "enabled" : "disabled", enable ? "disabling FPS limit" : "keeping current FPS limit");
    // Disallow vsync when fps limit is set
    this->vsync_enabled_ = enable;
    if (this->vsync_enabled_) {
        this->frame_limit_ = 0;
    }
    this->apply_sync_settings();
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
    SPDLOG_DEBUG("Applying sync settings with V-sync='{}' and frame_limit='{}'", this->vsync_enabled_, this->frame_limit_);
    this->window_.setVerticalSyncEnabled(this->vsync_enabled_);
    this->window_.setFramerateLimit(this->vsync_enabled_ ? 0u : this->frame_limit_);
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
