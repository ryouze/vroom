/**
 * @file backend.hpp
 *
 * @brief SFML window abstraction.
 */

#pragma once

#include <functional>  // for std::function
#include <vector>      // for std::vector

#include <SFML/Graphics.hpp>

namespace core::backend {

/**
 * @brief SFML window abstraction class with automatic settings management.
 *
 * On construction, the window is created based on the settings defined in "settings.hpp".
 *
 * @note To apply setting changes at runtime, modify the values in "settings.hpp" and call "recreate()".
 */
class Window {
  public:
    using event_callback_type = std::function<void(const sf::Event &)>;
    using update_callback_type = std::function<void(const float)>;
    using render_callback_type = std::function<void(sf::RenderWindow &)>;

    /**
     * @brief Construct a new SFML window based on current settings.
     *
     * This reads configuration from "settings.hpp" and creates the window with appropriate video mode, anti-aliasing, frame rate settings, and window constraints.
     *
     * @note The window is immediately ready for use. To change settings later, modify values in "settings.hpp" and call "recreate()".
     */
    explicit Window();

    /**
     * @brief Default destructor.
     */
    ~Window() = default;

    // Disable copy semantics - Window manages unique SFML resources
    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    // Allow move construction but disable move assignment for safety
    Window(Window &&) = default;
    Window &operator=(Window &&) = delete;

    /**
     * @brief Recreate the window with current settings from "settings.hpp".
     *
     * This closes the existing window and creates a new one with updated configuration.
     *
     * @note This causes a brief window flicker during recreation, as the old window is closed and a new one is created.
     *
     * @details This is an alias for the "create()" method. The goal is to make the class more intuitive to use.
     */
    void recreate();

    /**
     * @brief Get direct access to the underlying SFML RenderWindow.
     *
     * @return Mutable reference to the SFML RenderWindow instance.
     */
    [[nodiscard]] sf::RenderWindow &raw() { return this->window_; }

    /**
     * @brief Get read-only access to the underlying SFML RenderWindow.
     *
     * @return Read-only reference to the SFML RenderWindow instance.
     */
    [[nodiscard]] const sf::RenderWindow &raw() const { return this->window_; }

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

    /**
     * @brief Reference to all available fullscreen video modes (resolutions).
     *
     * This is sorted from best to worst, so an index of "0" will always give the best resolution and bits-per-pixel.
     */
    const std::vector<sf::VideoMode> &available_fullscreen_modes = sf::VideoMode::getFullscreenModes();

  private:
    /**
     * @brief Create the SFML window with the current settings.
     *
     * If the window already exists, it will be closed and recreated with the new settings.
     *
     * Settings used (not an exhaustive list):
     * - settings::current::anti_aliasing_idx
     * - settings::current::fullscreen
     * - settings::current::mode_idx
     */
    void create();

    /**
     * @brief The underlying SFML RenderWindow instance.
     *
     * @note This can be accessed using the "raw()" method.
     */
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

}  // namespace core::backend
