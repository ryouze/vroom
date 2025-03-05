/**
 * @file debug.cpp
 */

#include <cstddef>  // for std::size_t

#include <SFML/Graphics.hpp>

#include "debug.hpp"

namespace core::graphics::debug {

IndicatorRelative::IndicatorRelative(const sf::Vector2u &window_size,
                                     const sf::Color &color,
                                     const float radius,
                                     const std::size_t point_count,
                                     const float line_thickness)
    : color_(color),
      radius_(radius),
      point_count_(point_count),
      line_thickness_(line_thickness)
{
    this->update(window_size);
}

void IndicatorRelative::update(const sf::Vector2u &window_size)
{
    // Create a float version of the window size, required for "setPosition"
    sf::Vector2f window_size_f(static_cast<float>(window_size.x), static_cast<float>(window_size.y));

    // Create four circles, one for each corner of the window
    this->circles_[0] = sf::CircleShape(this->radius_, this->point_count_);
    this->circles_[0].setFillColor(this->color_);
    this->circles_[0].setOrigin({this->radius_, this->radius_});
    this->circles_[0].setPosition({0.f, 0.f});

    this->circles_[1] = sf::CircleShape(this->radius_, this->point_count_);
    this->circles_[1].setFillColor(this->color_);
    this->circles_[1].setOrigin({this->radius_, this->radius_});
    this->circles_[1].setPosition({window_size_f.x, 0.f});

    this->circles_[2] = sf::CircleShape(this->radius_, this->point_count_);
    this->circles_[2].setFillColor(this->color_);
    this->circles_[2].setOrigin({this->radius_, this->radius_});
    this->circles_[2].setPosition({0.f, window_size_f.y});

    this->circles_[3] = sf::CircleShape(this->radius_, this->point_count_);
    this->circles_[3].setFillColor(this->color_);
    this->circles_[3].setOrigin({this->radius_, this->radius_});
    this->circles_[3].setPosition({window_size_f.x, window_size_f.y});

    // Create four lines, one for each side of the window
    this->lines_[0] = sf::RectangleShape({window_size_f.x, this->line_thickness_});
    this->lines_[0].setFillColor(this->color_);
    this->lines_[0].setOrigin({0.f, this->line_thickness_ / 2.f});
    this->lines_[0].setPosition({0.f, 0.f});

    this->lines_[1] = sf::RectangleShape({window_size_f.x, this->line_thickness_});
    this->lines_[1].setFillColor(this->color_);
    this->lines_[1].setOrigin({0.f, this->line_thickness_ / 2.f});
    this->lines_[1].setPosition({0.f, window_size_f.y});

    this->lines_[2] = sf::RectangleShape({this->line_thickness_, window_size_f.y});
    this->lines_[2].setFillColor(this->color_);
    this->lines_[2].setOrigin({this->line_thickness_ / 2.f, 0.f});
    this->lines_[2].setPosition({0.f, 0.f});

    this->lines_[3] = sf::RectangleShape({this->line_thickness_, window_size_f.y});
    this->lines_[3].setFillColor(this->color_);
    this->lines_[3].setOrigin({this->line_thickness_ / 2.f, 0.f});
    this->lines_[3].setPosition({window_size_f.x, 0.f});
}

void IndicatorRelative::draw(sf::RenderWindow &window) const
{
    for (const auto &corner : this->circles_) {
        window.draw(corner);
    }
    for (const auto &line : this->lines_) {
        window.draw(line);
    }
}

IndicatorAbsolute::IndicatorAbsolute(const sf::Vector2u &window_size,
                                     const sf::Color &color,
                                     const float radius,
                                     const std::size_t point_count,
                                     const float line_thickness)
{
    // Create a float version of the window size, required for "setPosition"
    sf::Vector2f window_size_f(static_cast<float>(window_size.x), static_cast<float>(window_size.y));

    this->circles_[0] = sf::CircleShape(radius, point_count);
    this->circles_[0].setFillColor(color);
    this->circles_[0].setOrigin({radius, radius});
    this->circles_[0].setPosition({0.f, 0.f});

    this->circles_[1] = sf::CircleShape(radius, point_count);
    this->circles_[1].setFillColor(color);
    this->circles_[1].setOrigin({radius, radius});
    this->circles_[1].setPosition({window_size_f.x, 0.f});

    this->circles_[2] = sf::CircleShape(radius, point_count);
    this->circles_[2].setFillColor(color);
    this->circles_[2].setOrigin({radius, radius});
    this->circles_[2].setPosition({0.f, window_size_f.y});

    this->circles_[3] = sf::CircleShape(radius, point_count);
    this->circles_[3].setFillColor(color);
    this->circles_[3].setOrigin({radius, radius});
    this->circles_[3].setPosition({window_size_f.x, window_size_f.y});

    this->lines_[0] = sf::RectangleShape({window_size_f.x, line_thickness});
    this->lines_[0].setFillColor(color);
    this->lines_[0].setOrigin({0.f, line_thickness / 2.f});
    this->lines_[0].setPosition({0.f, 0.f});

    this->lines_[1] = sf::RectangleShape({window_size_f.x, line_thickness});
    this->lines_[1].setFillColor(color);
    this->lines_[1].setOrigin({0.f, line_thickness / 2.f});
    this->lines_[1].setPosition({0.f, window_size_f.y});

    this->lines_[2] = sf::RectangleShape({line_thickness, window_size_f.y});
    this->lines_[2].setFillColor(color);
    this->lines_[2].setOrigin({line_thickness / 2.f, 0.f});
    this->lines_[2].setPosition({0.f, 0.f});

    this->lines_[3] = sf::RectangleShape({line_thickness, window_size_f.y});
    this->lines_[3].setFillColor(color);
    this->lines_[3].setOrigin({line_thickness / 2.f, 0.f});
    this->lines_[3].setPosition({window_size_f.x, 0.f});
};

void IndicatorAbsolute::draw(sf::RenderWindow &window) const
{
    for (const auto &corner : this->circles_) {
        window.draw(corner);
    }
    for (const auto &line : this->lines_) {
        window.draw(line);
    }
}

}  // namespace core::graphics::debug
