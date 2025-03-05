/**
 * @file debug.hpp
 *
 * @brief Debugging graphics components.
 */

#pragma once

#include <array>    // for std::array
#include <cstddef>  // for std::size_t

#include <SFML/Graphics.hpp>

namespace core::graphics::debug {

class IndicatorRelative {
  public:
    explicit IndicatorRelative(const sf::Vector2u &window_size,
                               const sf::Color &color = sf::Color::Red,
                               const float radius = 10.f,
                               const std::size_t point_count = 8,  // Use 8 points for a sharp circle
                               const float line_thickness = 2.f);

    void update(const sf::Vector2u &window_size);

    void draw(sf::RenderWindow &window) const;

  private:
    const sf::Color color_;
    const float radius_;
    const std::size_t point_count_;
    const float line_thickness_;
    std::array<sf::CircleShape, 4> circles_;
    std::array<sf::RectangleShape, 4> lines_;
};

class IndicatorAbsolute {
  public:
    explicit IndicatorAbsolute(const sf::Vector2u &window_size,
                               const sf::Color &color = sf::Color::Green,
                               const float radius = 10.f,
                               const std::size_t point_count = 8,
                               const float line_thickness = 2.f);

    void draw(sf::RenderWindow &window) const;

  private:
    std::array<sf::CircleShape, 4> circles_;
    std::array<sf::RectangleShape, 4> lines_;
};

}  // namespace core::graphics::debug
