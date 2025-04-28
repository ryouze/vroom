/**
 * @file backend.cpp
 */

#include <algorithm>  // for std::clamp
#include <limits>     // for std::numeric_limits
#include <optional>   // for std::optional

#include <SFML/Graphics.hpp>
#include <spdlog/spdlog.h>

#include "backend.hpp"

namespace core::backend {

Window::Window(const WindowConfig &config)
    : config_(config),
      is_fullscreen_(config.start_fullscreen),
      window_()
{
    const sf::VideoMode boot_mode = this->is_fullscreen_
                                        ? sf::VideoMode::getDesktopMode()
                                        : sf::VideoMode{config.windowed_resolution};
    const sf::State boot_state = this->is_fullscreen_
                                     ? sf::State::Fullscreen
                                     : sf::State::Windowed;
    this->create_window(boot_mode, boot_state);
}

void Window::set_fullscreen(const bool enable,
                            const sf::VideoMode &mode)
{
    if (enable == this->is_fullscreen_) {
        return;
    }

    if (enable) {  // To full-screen
        this->config_.windowed_resolution = this->window_.getSize();
        this->is_fullscreen_ = true;
        this->recreate_window(mode, sf::State::Fullscreen);
    }
    else {  // To windowed
        this->is_fullscreen_ = false;
        const sf::VideoMode win_mode{this->config_.windowed_resolution};
        this->recreate_window(win_mode, sf::State::Windowed);
    }
}

void Window::set_windowed_resolution(const sf::Vector2u &res)
{
    if (this->is_fullscreen_) {
        return;
    }
    this->config_.windowed_resolution = res;
    this->recreate_window(sf::VideoMode{res}, sf::State::Windowed);
}

void Window::set_frame_limit(const unsigned fps_limit)
{
    this->config_.vertical_sync = false;
    this->config_.frame_limit = fps_limit;
    this->apply_sync_settings();
}

void Window::set_vertical_sync(const bool enable)
{
    this->config_.vertical_sync = enable;
    if (enable) {
        this->config_.frame_limit = 0;
    }
    this->apply_sync_settings();
}

void Window::run(const event_callback_type &on_event,
                 const update_callback_type &on_update,
                 const render_callback_type &on_render)
{
    sf::Clock clock;
    while (this->window_.isOpen()) {
        // Alow user of this call to explicitly handle events themselves}
        while (const std::optional event = this->window_.pollEvent()) {
            on_event(*event);
        }

        // Prevent extreme dt values at below 10 FPS by clamping between 0.001 and 0.1 seconds
        constexpr float dt_min = std::numeric_limits<float>::epsilon();
        constexpr float dt_max = 0.1F;
        const float dt = std::clamp(clock.restart().asSeconds(), dt_min, dt_max);

        on_update(dt);
        on_render(this->window_, dt);
    }
}

void Window::create_window(const sf::VideoMode &mode,
                           const sf::State state)
{
    // TODO: Set anti-aliasing level to 8x
    this->window_.create(mode, this->config_.title, state);
    this->window_.setMinimumSize(this->config_.minimum_size);
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
    if (this->config_.vertical_sync) {
        this->window_.setVerticalSyncEnabled(true);
        this->window_.setFramerateLimit(0);
    }
    else {
        this->window_.setVerticalSyncEnabled(false);
        this->window_.setFramerateLimit(this->config_.frame_limit);
    }
}

}  // namespace core::backend
