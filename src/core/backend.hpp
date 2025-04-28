/**
 * @file backend.hpp
 *
 * @brief SFML window abstraction.
 */

#pragma once

#include <format>      // for std::format
#include <functional>  // for std::function
#include <string>      // for std::string

#include <SFML/Graphics.hpp>

#include "generated.hpp"

namespace core::backend {

// Static configuration
struct WindowConfig {
    sf::Vector2u minimum_size = {800, 600};
    sf::Vector2u windowed_resolution = {1280, 720};
    std::string title = std::format("{} ({})", generated::PROJECT_NAME, generated::PROJECT_VERSION);
    unsigned frame_limit = 144;  // 0 == uncapped
    bool vsync = false;
    bool start_fullscreen = true;
    unsigned anti_aliasing_level = 8;
    // TODO: Allow user to change anti-aliasing level
};

class Window {
  public:
    using event_callback_type = std::function<void(const sf::Event &)>;
    using update_callback_type = std::function<void(const float)>;
    using render_callback_type = std::function<void(sf::RenderWindow &, const float)>;

    explicit Window(const WindowConfig &config = WindowConfig());  // Use default config

    // Default destructor
    ~Window() = default;

    // TODO: Move this to .cpp once the API is stable
    [[nodiscard]] bool is_fullscreen() const { return this->fullscreen_; }
    [[nodiscard]] bool is_vsync_enabled() const { return this->config_.vsync; }
    [[nodiscard]] const sf::ContextSettings &get_settings() const { return this->window_.getSettings(); }
    [[nodiscard]] sf::RenderWindow &raw() { return this->window_; }
    [[nodiscard]] const sf::RenderWindow &raw() const { return this->window_; }
    [[nodiscard]] sf::Vector2u get_size() const { return this->window_.getSize(); }

    void request_focus() { this->window_.requestFocus(); }
    void set_view(const sf::View &view) { this->window_.setView(view); }
    void clear(const sf::Color &color) { this->window_.clear(color); }
    template <typename D>
    void draw(const D &drawable) { this->window_.draw(drawable); }
    void display() { this->window_.display(); }
    void close() { this->window_.close(); }

    void set_fullscreen(const bool enable,
                        const sf::VideoMode &video_mode = sf::VideoMode::getDesktopMode());

    // void set_windowed_resolution(const sf::Vector2u &resolution);

    // FPS or V-sync (exclusive)
    void set_fps_limit(const unsigned fps_limit);  // 0 == uncapped
    void set_vsync(const bool enable);

    // Main-loop helper
    void run(const event_callback_type &on_event,
             const update_callback_type &on_update,
             const render_callback_type &on_render);

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    Window(Window &&) = default;
    Window &operator=(Window &&) = default;

  private:
    void create_window(const sf::VideoMode &mode,
                       const sf::State state);
    void recreate_window(const sf::VideoMode &mode,
                         const sf::State state);
    void apply_sync_settings();

    WindowConfig config_;
    bool fullscreen_;
    sf::RenderWindow window_;
};

}  // namespace core::backend
