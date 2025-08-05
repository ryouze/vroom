/**
 * @file input.cpp
 */

#include <algorithm>  // for std::max, std::clamp
#include <cstdlib>    // for std::abs

#include <SFML/Window/Joystick.hpp>
#include <spdlog/spdlog.h>

#include "input.hpp"
#include "settings.hpp"

namespace core::input {

Gamepad::Gamepad(const unsigned int id,
                 const float deadzone)
    : id_{id},
      deadzone_{deadzone}
{
    SPDLOG_DEBUG("Initialized Gamepad with ID '{}' and deadzone '{}'", this->id_, this->deadzone_);
}

bool Gamepad::is_connected() const
{
    return sf::Joystick::isConnected(this->id_);
}

float Gamepad::get_gas() const
{
    // Cast to axis from settings
    const sf::Joystick::Axis gas_axis = static_cast<sf::Joystick::Axis>(settings::current::gamepad_gas_axis);

    // If not available, exit early
    if (!sf::Joystick::hasAxis(this->id_, gas_axis)) {
        return 0.0f;
    }

    // Get the axis value, take only negative values and flip it
    const float axis_value = sf::Joystick::getAxisPosition(this->id_, gas_axis);
    float normalized_value = std::max(0.0f, -axis_value) / 100.0f;

    // Apply deadzone to prevent accidental input
    normalized_value = this->apply_deadzone(normalized_value);

    // If flipped in settings, flip the value again
    return settings::current::gamepad_invert_gas ? (1.0f - normalized_value) : normalized_value;
}

float Gamepad::get_brake() const
{
    // Cast to axis from settings
    const sf::Joystick::Axis brake_axis = static_cast<sf::Joystick::Axis>(settings::current::gamepad_brake_axis);

    // If not available, exit early
    if (!sf::Joystick::hasAxis(this->id_, brake_axis)) {
        return 0.0f;
    }

    // Get the axis value, take only positive values
    const float axis_value = sf::Joystick::getAxisPosition(this->id_, brake_axis);
    float normalized_value = std::max(0.0f, axis_value) / 100.0f;

    // Apply deadzone to prevent accidental input
    normalized_value = this->apply_deadzone(normalized_value);

    // If flipped in settings, flip the value
    return settings::current::gamepad_invert_brake ? (1.0f - normalized_value) : normalized_value;
}

float Gamepad::get_steer() const
{
    // Cast to axis from settings
    const sf::Joystick::Axis steering_axis = static_cast<sf::Joystick::Axis>(settings::current::gamepad_steering_axis);

    // If not available, exit early
    if (!sf::Joystick::hasAxis(this->id_, steering_axis)) {
        return 0.0f;
    }

    // Get the axis value, take all values
    const float percent = sf::Joystick::getAxisPosition(this->id_, steering_axis);
    float normalized_value = std::clamp(percent / 100.0f, -1.0f, 1.0f);

    // Apply deadzone to prevent accidental input
    normalized_value = this->apply_deadzone(normalized_value);

    // If flipped in settings, flip the value
    return settings::current::gamepad_invert_steering ? -normalized_value : normalized_value;
}

bool Gamepad::get_handbrake() const
{
    // Cast to unsigned int from settings
    const unsigned int button_index = static_cast<unsigned int>(settings::current::gamepad_handbrake_button);
    return sf::Joystick::getButtonCount(this->id_) > button_index &&
           sf::Joystick::isButtonPressed(this->id_, button_index);
}

unsigned int Gamepad::get_button_count() const
{
    return sf::Joystick::getButtonCount(this->id_);
}

float Gamepad::apply_deadzone(const float value) const
{
    // If the value is within the deadzone, return 0.0f
    // This is essentially applying the deadzone, as advertised
    if (std::abs(value) < this->deadzone_) {
        return 0.0f;
    }

    // Determine the sign of the input value to preserve its direction (positive or negative)
    const float sign = (value > 0.0f) ? 1.0f : -1.0f;

    // Now, without scaling, the remaining usable range would be compressed and unusable
    // E.g., if deadzone is "0.2", then inputs from "0.0" to "0.2" are ignored, leaving only "0.2" to "1.0" as the usable range (80% of the original range)
    //
    // Thus, we need to stretch the remaining 80% range back to fill the full 100% output range so the user can still achieve maximum values
    // Step 1: Remove the deadzone portion by subtracting it from the absolute input value (this gives us the "useful" input above the deadzone threshold)
    // Step 2: Divide by the remaining range ("1.0" - deadzone) to scale it back to the full "0.0" to "1.0" range
    // E.g., if deadzone="0.2" and input="0.5" -> (0.5 - 0.2) / (1.0 - 0.2) = 0.3 / 0.8 = 0.375
    // E.g., if deadzone="0.2" and input="1.0" -> (1.0 - 0.2) / (1.0 - 0.2) = 0.8 / 0.8 = 1.0 (full output preserved)
    return sign * ((std::abs(value) - this->deadzone_) / (1.0f - this->deadzone_));
}

}  // namespace core::input
