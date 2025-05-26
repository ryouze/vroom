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

// TODO: Add spdlog everywhere
Window::Window()
    : fullscreen_(default_start_fullscreen_),
      vsync_enabled_(default_vsync_enabled_),
      frame_limit_(default_frame_limit_),
      windowed_resolution_(default_windowed_resolution_)
{
    const sf::VideoMode initial_mode = this->fullscreen_
                                           ? sf::VideoMode::getDesktopMode()
                                           : sf::VideoMode{this->windowed_resolution_};
    const sf::State initial_state = this->fullscreen_
                                        ? sf::State::Fullscreen
                                        : sf::State::Windowed;
    this->create_window(initial_mode, initial_state);
}

void Window::set_window(const bool enable,
                        const sf::VideoMode &mode)
{
    // Only skip if disabling when already windowed
    if (!enable && !this->fullscreen_) {
        return;
    }

    this->fullscreen_ = enable;
    if (this->fullscreen_) {  // To fullscreen
        // Always recreate when enabling, even if already fullscreen
        this->recreate_window(mode, sf::State::Fullscreen);
    }
    else {  // To windowed
        sf::VideoMode windowed_mode{this->windowed_resolution_};
        this->recreate_window(windowed_mode, sf::State::Windowed);
    }
}

void Window::set_fps_limit(const unsigned fps_limit)
{
    // Disallow fps limit when vsync is on
    this->vsync_enabled_ = false;
    this->frame_limit_ = fps_limit;
    this->apply_sync_settings();
}

void Window::set_vsync(const bool enable)
{
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
    sf::Clock clock;
    while (this->window_.isOpen()) {
        // Alow user of this call to explicitly handle events themselves
        this->window_.handleEvents(on_event);
        // Prevent extreme dt by clamping to 0.1 seconds
        constexpr float dt_max = 0.1f;
        const float dt = std::min(clock.restart().asSeconds(), dt_max);
        on_update(dt);
        on_render(this->window_);
    }
}

void Window::create_window(const sf::VideoMode &mode,
                           const sf::State state)
{
    const sf::ContextSettings settings{.antiAliasingLevel = this->default_anti_aliasing_level_};
    this->window_.create(mode, this->default_title_, state, settings);
    this->window_.setMinimumSize(this->default_minimum_size_);
    this->apply_sync_settings();
}

void Window::recreate_window(const sf::VideoMode &mode,
                             const sf::State state)
{
    this->window_.close();
    this->create_window(mode, state);
}

void Window::apply_sync_settings()
{
    this->window_.setVerticalSyncEnabled(this->vsync_enabled_);
    this->window_.setFramerateLimit(this->vsync_enabled_ ? 0u : this->frame_limit_);
}

}  // namespace core::backend
