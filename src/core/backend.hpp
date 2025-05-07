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

// TODO: Use this in "set_window()"
// enum class WindowState {
//     Fullscreen,
//     Windowed,
// };

class Window {
    // TODO: Get rid of bits per pixel, always set resolution in object's constructor and rely on the default bits per pixel
  public:
    using event_callback_type = std::function<void(const sf::Event &)>;
    using update_callback_type = std::function<void(const float)>;
    using render_callback_type = std::function<void(sf::RenderWindow &)>;

    // TODO: Set initial screen resolution, fps limit vsync, AA, etc. from config file on disk
    explicit Window();

    // Default destructor
    ~Window() = default;

    // Do not put these simple getters/setters in .cpp
    [[nodiscard]] bool is_fullscreen() const { return this->fullscreen_; }
    [[nodiscard]] bool is_vsync_enabled() const { return this->vsync_enabled_; }
    [[nodiscard]] sf::RenderWindow &raw() { return this->window_; }
    [[nodiscard]] const sf::RenderWindow &raw() const { return this->window_; }
    [[nodiscard]] sf::Vector2u get_resolution() const { return this->window_.getSize(); }
    void request_focus() { this->window_.requestFocus(); }
    void set_view(const sf::View &view) { this->window_.setView(view); }
    void clear(const sf::Color &color) { this->window_.clear(color); }
    template <typename Drawable>
    void draw(const Drawable &drawable) { this->window_.draw(drawable); }
    void display() { this->window_.display(); }
    void close() { this->window_.close(); }

    void set_window(const bool enable,  // <-- Replace this with enum
                    const sf::VideoMode &mode = sf::VideoMode::getDesktopMode());

    // FPS or V-sync (exclusive)
    void set_fps_limit(const unsigned fps_limit);
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

    // Fixed defaults
    static constexpr sf::Vector2u default_windowed_resolution_ = {1280, 720};
    static constexpr sf::Vector2u default_minimum_size_ = {800, 600};
    static constexpr unsigned default_bits_per_pixel_ = 32;
    static inline const std::string default_title_ = std::format("{} ({})", generated::PROJECT_NAME, generated::PROJECT_VERSION);
    static constexpr unsigned default_frame_limit_ = 144;
    static constexpr bool default_vsync_enabled_ = false;
    static constexpr bool default_start_fullscreen_ = true;
    static constexpr unsigned default_anti_aliasing_level_ = 8;

    // Runtime state; always initialize in initializer list
    bool fullscreen_;
    bool vsync_enabled_;
    unsigned frame_limit_;
    sf::Vector2u windowed_resolution_;

    sf::RenderWindow window_;
};

}  // namespace core::backend
