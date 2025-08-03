/**
 * @file window.hpp
 *
 * @brief SFML window abstraction.
 */

#pragma once

#include <SFML/Graphics.hpp>

#include "generated.hpp"
#include "settings.hpp"

namespace core::window {

/**
 * @brief SFML window abstraction class.
 *
 * On construction, the window is created based on the settings defined in "settings.hpp".
 *
 * The user can modify the settings inside the "settings.hpp" file and then call the "recreate()" method to make this class read these changes and apply them to the window.
 */
class Window {
  public:
    using event_callback_type = std::function<void(const sf::Event &)>;
    using update_callback_type = std::function<void(const float)>;
    using render_callback_type = std::function<void(sf::RenderWindow &)>;

    /**
     * @brief Construct a new SFML window.
     *
     * On construction, the window is created based on the settings defined in "settings.hpp".
     *
     * @note If any of these settings change (e.g., resolution, fullscreen mode, vsync, FPS limit), call the "recreate()" method to apply the changes.
     */
    explicit Window();

    /**
     * @brief Default destructor.
     */
    ~Window() = default;

    /**
     * @brief Recreate the window with current settings - resolution, fullscreen mode, vsync, FPS limit, etc.
     *
     * @note This is a simple alias to the internal "create()" method, keeping the code more readable by separating "create" and "recreate" actions.
     */
    void recreate();

    // Get the window instance itself
    [[nodiscard]] sf::RenderWindow &raw() { return this->window_; }
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

    // TODO: Find out why these deleters fail to compile

    // Window(const Window &) = delete;
    // Window &operator=(const Window &) = delete;

    // Window(Window &&) = default;
    // Window &operator=(Window &&) = default;

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
     * Settings used:
     * - settings::constants::anti_aliasing_level
     * - settings::current::fullscreen
     * - settings::current::mode_idx
     * - settings::constants::windowed_width
     * - settings::constants::windowed_height
     *
     * // TODO: Make sure all the variables used in this method are listed above. Perhaps write a Python script with regex.
     */
    void create();

    /**
     * @brief An instance of the SFML RenderWindow.
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

}  // namespace core::window
