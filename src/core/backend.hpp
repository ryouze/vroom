/**
 * @file backend.hpp
 *
 * @brief SFML window abstraction.
 */

#pragma once

#include <memory>  // for std::unique_ptr

#include <SFML/Graphics.hpp>

namespace core::backend {

/**
 * @brief Create a new SFML window with sane defaults from the internal "default_config" namespace.
 *
 * On Windows, this also sets the titlebar icon.
 *
 * @return Smart pointer to the created window. The ownership is transferred to the caller. The caller is also responsible for adding an event loop and rendering logic.
 */
[[nodiscard]] std::unique_ptr<sf::RenderWindow> create_window();

/**
 * @brief Set the current FPS limit.
 *
 * @param window Window to apply the settings to.
 * @param fps_limit New FPS limit (e.g., "60u").
 *
 * @note This also sets the internal "current_config::fps_limit" variable to the new value.
 */
void set_fps_limit(sf::RenderWindow &window,
                   const unsigned int fps_limit);

/**
 * @brief Get the current FPS limit.
 *
 * @return Current FPS limit (e.g., "60u").
 *
 * @note This is the value of the internal "current_config::fps_limit" and is set by "set_fps_limit()".
 */
[[nodiscard]] unsigned int get_fps_limit() noexcept;

}  // namespace core::backend
