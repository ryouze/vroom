/**
 * @file backend.cpp
 */

#include <algorithm>  // for std::clamp
#include <optional>   // for std::optional

#include <SFML/Graphics.hpp>
#include <spdlog/spdlog.h>
#ifndef NDEBUG  // Debug, remove later
#include <imgui.h>
#endif

#include "backend.hpp"

namespace core::backend {

// TODO: Add spdlog everywhere
Window::Window(const WindowConfig &config)
    : config_(config),
      fullscreen_(config.start_fullscreen)
{
    const sf::VideoMode initial_mode = this->fullscreen_
                                           ? sf::VideoMode::getDesktopMode()
                                           : sf::VideoMode{config.windowed_resolution};
    const sf::State initial_state = this->fullscreen_
                                        ? sf::State::Fullscreen
                                        : sf::State::Windowed;
    this->create_window(initial_mode, initial_state);
}

void Window::set_fullscreen(const bool enable,
                            const sf::VideoMode &video_mode)
{
    // Not sure if this should be here!
    // if (enable == this->fullscreen_) {
    //     return;
    // }
    if (enable) {  // To fullscreen
        this->config_.windowed_resolution = this->window_.getSize();
        this->fullscreen_ = true;
        this->recreate_window(video_mode, sf::State::Fullscreen);
    }
    else {  // To windowed
        this->fullscreen_ = false;
        const sf::VideoMode windowed_mode{this->config_.windowed_resolution};
        this->recreate_window(windowed_mode, sf::State::Windowed);
    }
}

// void Window::set_windowed_resolution(const sf::Vector2u &resolution)
// {
//     if (this->fullscreen_) {
//         return;
//     }
//     this->config_.windowed_resolution = resolution;
//     this->recreate_window(sf::VideoMode{resolution}, sf::State::Windowed);
// }

// TODO: Add checks and warnings if tried to enable both at the same time
void Window::set_fps_limit(const unsigned fps_limit)
{
    this->config_.vsync = false;
    this->config_.frame_limit = fps_limit;
    this->apply_sync_settings();
}

void Window::set_vsync(const bool enable)
{
    this->config_.vsync = enable;
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
        // Alow user of this call to explicitly handle events themselves
        this->window_.handleEvents(on_event);
        // Prevent extreme dt values at below 10 FPS by clamping between 0.001 and 0.1 seconds
        constexpr float dt_min = 0.001f;
        constexpr float dt_max = 0.1f;
        const float dt = std::clamp(clock.restart().asSeconds(), dt_min, dt_max);
        on_update(dt);

#ifndef NDEBUG  // Debug, remove later
        const ImGuiViewport *vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + vp->WorkSize.x, vp->WorkPos.y), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(320.0f, 360.0f), ImGuiCond_Always);
        if (ImGui::Begin("Window Debugging", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground)) {
            ImGui::TextUnformatted("Config:");
            ImGui::BulletText(".minimum_size: %dx%d", this->config_.minimum_size.x, this->config_.minimum_size.y);
            ImGui::BulletText(".windowed_resolution: %dx%d", this->config_.windowed_resolution.x, this->config_.windowed_resolution.y);
            ImGui::BulletText(".title: %s", this->config_.title.c_str());
            ImGui::BulletText(".frame_limit: %u", this->config_.frame_limit);
            ImGui::BulletText(".vsync: %s", this->config_.vsync ? "true" : "false");
            ImGui::BulletText(".start_fullscreen: %s", this->config_.start_fullscreen ? "true" : "false");
            ImGui::BulletText(".anti_aliasing_level: %u", this->config_.anti_aliasing_level);
            ImGui::TextUnformatted("Member Variables:");
            ImGui::BulletText("this->fullscreen_: %s", this->fullscreen_ ? "true" : "false");
            const auto window_settings = this->window_.getSettings();
            const auto window_size_u = this->window_.getSize();
            ImGui::TextUnformatted("SFML:");
            ImGui::BulletText("Current Resolution: %dx%d", window_size_u.x, window_size_u.y);
            ImGui::BulletText("Current Anti-Aliasing Level: %d", window_settings.antiAliasingLevel);
            ImGui::BulletText("Current OpenGL Version: %d.%d", window_settings.majorVersion, window_settings.minorVersion);
        }
        ImGui::End();
#endif
        on_render(this->window_, dt);
    }
}

void Window::create_window(const sf::VideoMode &mode,
                           const sf::State state)
{
    // Set anti-aliasing level to 8x
    const sf::ContextSettings context_settings = {.antiAliasingLevel = this->config_.anti_aliasing_level};
    this->window_.create(mode, this->config_.title, state, context_settings);
    this->window_.setMinimumSize(std::optional<sf::Vector2u>{this->config_.minimum_size});
    this->apply_sync_settings();
}

void Window::recreate_window(const sf::VideoMode &mode,
                             const sf::State state)
{
    // TODO: Re-creating window seems to break anti-aliasing?
    // And OpenGL gets weird too, as if it was random garbage value of int instead of actual value
    this->window_.close();
    this->create_window(mode, state);
}

void Window::apply_sync_settings()
{
    if (this->config_.vsync) {
        this->window_.setVerticalSyncEnabled(true);
        this->window_.setFramerateLimit(0);
    }
    else {
        this->window_.setVerticalSyncEnabled(false);
        this->window_.setFramerateLimit(this->config_.frame_limit);
    }
}

}  // namespace core::backend
