/**
 * @file window.hpp
 *
 * @brief SFML window abstraction.
 */

#pragma once

#include <format>      // for std::format
#include <functional>  // for std::function
#include <string>      // for std::string

#include <SFML/Graphics.hpp>

namespace core::window {

class Window {
  public:
    using event_callback_type = std::function<void(const sf::Event &)>;
    using update_callback_type = std::function<void(const float)>;
    using render_callback_type = std::function<void(sf::RenderWindow &)>;

    /**
     * @brief Construct window using settings from the centralized configuration.
     *
     * The window will be created with the current settings (fullscreen/windowed state, resolution, vsync, fps) from settings::current.
     */
    explicit Window();

    /**
     * @brief Initialize or reinitialize the window with current settings.
     *
     * This method handles both initial window creation and applying changes from the centralized configuration. It will recreate the window if necessary to apply the current settings properly.
     */
    void initialize_with_current_settings();

    // Default destructor
    ~Window() = default;

    // Do not put these simple getters/setters in .cpp
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

    /**
     * @brief Toggle fullscreen mode and reinitialize window with current settings.
     *
     * This method toggles between fullscreen and windowed mode using the current settings.
     */
    void toggle_fullscreen();

    /**
     * @brief Apply current settings from the centralized configuration.
     *
     * This method applies the fullscreen, vsync, and fps settings from settings::current to the window.
     * Should be called after configuration is loaded to ensure the window matches the saved settings.
     */
    void apply_current_settings();

    /**
     * @brief Get all available fullscreen video modes.
     *
     * @return Vector of available video modes sorted by resolution (best first).
     */
    [[nodiscard]] static std::vector<sf::VideoMode> get_available_modes();

    /**
     * @brief Run the main application loop with provided callbacks.
     *
     * @param on_event Callback function for handling SFML events.
     * @param on_update Callback function for updating game state (receives delta time).
     * @param on_render Callback function for rendering (receives render window reference).
     *
     * @note The loop continues until the window is closed. Delta time is clamped to prevent extreme values.
     */
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
    void apply_sync_settings();

    /**
     * @brief Get the video mode that should be used for the current settings.
     *
     * For fullscreen: returns the mode at settings::current::resolution_idx or desktop mode as fallback.
     * For windowed: returns the windowed resolution from settings.
     *
     * @return The appropriate video mode for current settings.
     */
    [[nodiscard]] sf::VideoMode get_video_mode_for_current_settings() const;

    sf::RenderWindow window_;
};

/**
 * @brief Convert an sf::Vector2u to sf::Vector2f.
 *
 * @param vector Vector to convert (e.g., {1280, 800}).
 *
 * @return Converted vector with floating-point values (e.g., {1280.f, 800.f}).
 */
[[nodiscard]] sf::Vector2f to_vector2f(const sf::Vector2u &vector);

}  // namespace core::window
