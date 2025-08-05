/**
 * @file input.hpp
 *
 * @brief Game input handling.
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SFML/Window/Joystick.hpp>

#include "settings.hpp"

namespace core::input {

/**
 * @brief Xbox controller input handler with configurable axes and buttons.
 *
 *
 * @note Always run "is_connected()" once per frame in the main game loop before using any input methods.
 */
class Gamepad {
  public:
    /**
     * @brief Construct a new Gamepad object.
     *
     * @param id Gamepad ID (default: "0").
     */
    explicit Gamepad(const unsigned int id = 0)
        : id_{id} {}

    /**
     * @brief Check if the gamepad is currently connected.
     *
     * @return True if the gamepad is connected, false otherwise.
     */
    [[nodiscard]] bool is_connected() const
    {
        return sf::Joystick::isConnected(this->id_);
    }

    /**
     * @brief Get the gas input from the configured axis.
     *
     * @return Value in the [0.0, 1.0] range, where "0" is no throttle and "1" is full throttle.
     */
    [[nodiscard]] float get_gas() const
    {
        // Cast to axis from settings
        const sf::Joystick::Axis gas_axis = static_cast<sf::Joystick::Axis>(settings::current::gamepad_gas_axis);

        // If not available, exit early
        if (!sf::Joystick::hasAxis(this->id_, gas_axis)) {
            return 0.0f;
        }

        // Get the axis value, take only negative values and flip it by default
        const float axis_value = sf::Joystick::getAxisPosition(this->id_, gas_axis);
        const float normalized_value = std::max(0.0f, -axis_value) / 100.0f;

        // If flipped in settings, flip the value again
        return settings::current::gamepad_invert_gas ? (1.0f - normalized_value) : normalized_value;
    }

    /**
     * @brief Get the brake input from the configured axis.
     *
     * @return Value in the [0.0, 1.0] range, where "0" is no braking and "1" is full brake.
     */
    [[nodiscard]] float get_brake() const
    {
        // Cast to axis from settings
        const sf::Joystick::Axis brake_axis = static_cast<sf::Joystick::Axis>(settings::current::gamepad_brake_axis);

        // If not available, exit early
        if (!sf::Joystick::hasAxis(this->id_, brake_axis)) {
            return 0.0f;
        }

        // Get the axis value, take only positive values and flip it by default
        const float axis_value = sf::Joystick::getAxisPosition(this->id_, brake_axis);
        const float normalized_value = std::max(0.0f, axis_value) / 100.0f;

        // If flipped in settings, flip the value again
        return settings::current::gamepad_invert_brake ? (1.0f - normalized_value) : normalized_value;
    }

    /**
     * @brief Get steering input from the configured axis.
     *
     * @return Value in the range [-1.0, 1.0] where "-1" is full left, "0" is center, and "1" is full right.
     */
    [[nodiscard]] float get_steer() const
    {
        // Cast to axis from settings
        const sf::Joystick::Axis steering_axis = static_cast<sf::Joystick::Axis>(settings::current::gamepad_steering_axis);

        // If not available, exit early
        if (!sf::Joystick::hasAxis(this->id_, steering_axis)) {
            return 0.0f;
        }

        // Get the axis value, take all values
        const float percent = sf::Joystick::getAxisPosition(this->id_, steering_axis);
        const float normalized_value = std::clamp(percent / 100.0f, -1.0f, 1.0f);

        // If flipped in settings, flip the value
        return settings::current::gamepad_invert_steering ? -normalized_value : normalized_value;
    }

    /**
     * @brief Get handbrake input from the configured button.
     *
     * @return True if the button is pressed, false otherwise.
     */
    [[nodiscard]] bool get_handbrake() const
    {
        // Cast to unsigned int from settings
        const unsigned int button_index = static_cast<unsigned int>(settings::current::gamepad_handbrake_button);
        return sf::Joystick::getButtonCount(this->id_) > button_index &&
               sf::Joystick::isButtonPressed(this->id_, button_index);
    }

    /**
     * @brief Get the number of available buttons on the gamepad.
     *
     * @return Number of supported buttons.
     */
    [[nodiscard]] unsigned int get_button_count() const
    {
        return sf::Joystick::getButtonCount(this->id_);
    }

  private:
    /**
     * @brief Gamepad ID.
     */
    unsigned int id_;
};

}  // namespace core::input
