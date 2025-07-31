/**
 * @file colors.hpp
 *
 * @brief Background colors.
 */

#pragma once

#include <SFML/Graphics.hpp>

namespace core::colors {

// Background colors for each GameState
constexpr struct {
    sf::Color menu;
    sf::Color game;
    sf::Color settings;
} window{
    .menu = {210, 180, 140},
    .game = {200, 170, 130},
    .settings = {28, 28, 30},
};

}  // namespace core::colors
