/**
 * @file misc.cpp
 */

#include <SFML/Graphics.hpp>

#include "misc.hpp"

namespace core::misc {

sf::Vector2f to_vector2f(const sf::Vector2u &vector)
{
    return sf::Vector2f(static_cast<float>(vector.x), static_cast<float>(vector.y));
}

}  // namespace core::misc
