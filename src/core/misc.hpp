/**
 * @file misc.hpp
 *
 * @brief Miscellaneous utilities and helper functions.
 */

#pragma once

#include <SFML/Graphics.hpp>

namespace core::misc {

/**
 * @brief Convert an sf::Vector2u to sf::Vector2f.
 *
 * @param vector Vector to convert (e.g., {1280, 800}).
 * @return Converted vector with floating-point values (e.g., {1280.f, 800.f}).
 */
[[nodiscard]] sf::Vector2f to_vector2f(const sf::Vector2u &vector);

}  // namespace core::misc
