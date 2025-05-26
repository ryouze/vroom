/**
 * @file backend.cpp
 */

#include <algorithm>  // for std::min

#include <SFML/Graphics.hpp>
#include <spdlog/spdlog.h>
#ifndef NDEBUG  // Debug, remove later
#include <imgui.h>
#endif

#include "backend.hpp"

namespace core::backend {

Window::Window()
    : fullscreen_(default_start_fullscreen_),
      vsync_enabled_(default_vsync_enabled_),
      frame_limit_(default_frame_limit_),
      windowed_resolution_(default_windowed_resolution_)
{
    SPDLOG_DEBUG("Initializing window with fullscreen='{}', vsync='{}', frame_limit='{}', windowed_resolution='{}x{}'", this->fullscreen_, this->vsync_enabled_, this->frame_limit_, this->windowed_resolution_.x, this->windowed_resolution_.y);

    const sf::VideoMode initial_mode = this->fullscreen_
                                           ? sf::VideoMode::getDesktopMode()
                                           : sf::VideoMode{this->windowed_resolution_};
    const sf::State initial_state = this->fullscreen_
                                        ? sf::State::Fullscreen
                                        : sf::State::Windowed;
    this->create_window(initial_mode, initial_state);

    SPDLOG_DEBUG("Window initialized successfully in '{}' mode with resolution '{}x{}'", this->fullscreen_ ? "fullscreen" : "windowed", initial_mode.size.x, initial_mode.size.y);
}

void Window::set_window_state(const WindowState state,
                              const sf::VideoMode &mode)
{
    const bool target_fullscreen = (state == WindowState::Fullscreen);

    // Skip if already in target state and not changing fullscreen mode
    if (!target_fullscreen && !this->fullscreen_) {
        SPDLOG_DEBUG("Already in windowed mode, skipping state change!");
        return;
    }

    SPDLOG_DEBUG("Changing window state from '{}' to '{}'", this->fullscreen_ ? "fullscreen" : "windowed", target_fullscreen ? "fullscreen" : "windowed");

    this->fullscreen_ = target_fullscreen;
    if (this->fullscreen_) {  // To fullscreen
        SPDLOG_DEBUG("Switching to fullscreen with video mode '{}x{}'", mode.size.x, mode.size.y);
        this->recreate_window(mode, sf::State::Fullscreen);
    }
    else {  // To windowed
        const sf::VideoMode windowed_mode{this->windowed_resolution_};
        SPDLOG_DEBUG("Switching to windowed mode with resolution '{}x{}'", windowed_mode.size.x, windowed_mode.size.y);
        this->recreate_window(windowed_mode, sf::State::Windowed);
    }

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
    const sf::ContextSettings settings{.antiAliasingLevel = this->default_anti_aliasing_level_};
    this->window_.create(mode, this->default_title_, state, settings);
    this->window_.setMinimumSize(this->default_minimum_size_);
    this->apply_sync_settings();
    SPDLOG_DEBUG("Window created successfully with anti-aliasing level '{}'", this->default_anti_aliasing_level_);
}

void Window::recreate_window(const sf::VideoMode &mode,
                             const sf::State state)
{
    SPDLOG_DEBUG("Recreating window with video mode '{}x{}' in '{}' state", mode.size.x, mode.size.y, (state == sf::State::Fullscreen) ? "fullscreen" : "windowed");
    this->window_.close();
    this->create_window(mode, state);
}

void Window::apply_sync_settings()
{
    SPDLOG_DEBUG("Applying sync settings with V-sync='{}' and frame_limit='{}'", this->vsync_enabled_, this->frame_limit_);
    this->window_.setVerticalSyncEnabled(this->vsync_enabled_);
    this->window_.setFramerateLimit(this->vsync_enabled_ ? 0u : this->frame_limit_);
}

}  // namespace core::backend
