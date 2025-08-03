/**
 * @file gamepad.cpp
 */

#include <algorithm>  // for std::clamp, std::abs
#include <cmath>      // for std::abs

#include <SFML/Window/Joystick.hpp>

#include "gamepad.hpp"
#include "settings.hpp"

namespace core::gamepad {

Gamepad::Gamepad(const unsigned controller_id)
    : controller_id_(controller_id)
{
}

void Gamepad::update(const float dt)
{
    // Accumulate the delta time
    this->info_update_timer_ += dt;

    // If the accumulated time does not exceed the update rate, return early
    if (this->info_update_timer_ < this->info_update_rate) {
        return;
    }

    // Reset timer for next update cycle
    this->info_update_timer_ -= this->info_update_rate;  // Keep any overshoot

    // Refresh the cached info
    this->cached_info_ = GamepadInfo{};

    this->cached_info_.connected = sf::Joystick::isConnected(this->controller_id_);

    if (!this->cached_info_.connected) {
        this->cached_info_.name = "No controller connected";
        return;
    }

    // Get controller name
    const std::string name = sf::Joystick::getIdentification(this->controller_id_).name.toAnsiString();
    this->cached_info_.name = name.empty() ? "Generic Controller" : name;

    // Get button count
    this->cached_info_.button_count = sf::Joystick::getButtonCount(this->controller_id_);

    // Check which axes are available
    for (int axis = 0; axis < 8; ++axis) {
        if (sf::Joystick::hasAxis(this->controller_id_, static_cast<sf::Joystick::Axis>(axis))) {
            this->cached_info_.available_axes.push_back(axis);
        }
    }

    // Check if configured controls are available
    this->cached_info_.has_configured_steering_axis = sf::Joystick::hasAxis(this->controller_id_, static_cast<sf::Joystick::Axis>(settings::current::gamepad_steering_axis));
    this->cached_info_.has_configured_throttle_axis = sf::Joystick::hasAxis(this->controller_id_, static_cast<sf::Joystick::Axis>(settings::current::gamepad_throttle_axis));
    this->cached_info_.has_configured_handbrake_button = (settings::current::gamepad_handbrake_button < static_cast<int>(this->cached_info_.button_count));
}

const GamepadInfo &Gamepad::get_info() const
{
    return this->cached_info_;
}

GamepadInput Gamepad::get_input() const
{
    GamepadInput input;

    if (!sf::Joystick::isConnected(this->controller_id_)) {
        return input;  // All values remain 0/false
    }

    // Get steering input
    if (sf::Joystick::hasAxis(this->controller_id_, static_cast<sf::Joystick::Axis>(settings::current::gamepad_steering_axis))) {
        const float raw_steering = sf::Joystick::getAxisPosition(this->controller_id_, static_cast<sf::Joystick::Axis>(settings::current::gamepad_steering_axis));
        float normalized_steering = std::clamp(raw_steering / 100.0f, -1.0f, 1.0f);
        if (settings::current::gamepad_invert_steering) {
            normalized_steering = -normalized_steering;
        }
        input.steering = apply_deadzone(normalized_steering);
    }

    // Get throttle/brake input
    if (sf::Joystick::hasAxis(this->controller_id_, static_cast<sf::Joystick::Axis>(settings::current::gamepad_throttle_axis))) {
        const float raw_throttle = sf::Joystick::getAxisPosition(this->controller_id_, static_cast<sf::Joystick::Axis>(settings::current::gamepad_throttle_axis));
        float normalized_throttle = std::clamp(-raw_throttle / 100.0f, -1.0f, 1.0f);  // Flip and normalize
        if (settings::current::gamepad_invert_throttle) {
            normalized_throttle = -normalized_throttle;
        }

        if (normalized_throttle > 0.0f) {
            input.throttle = apply_deadzone(normalized_throttle);
        }
        else {
            input.brake = apply_deadzone(-normalized_throttle);
        }
    }

    // Get handbrake input
    if (settings::current::gamepad_handbrake_button < static_cast<int>(sf::Joystick::getButtonCount(this->controller_id_))) {
        input.handbrake = sf::Joystick::isButtonPressed(this->controller_id_, static_cast<unsigned>(settings::current::gamepad_handbrake_button));
    }

    return input;
}

float Gamepad::get_processed_axis_value(const int axis_index) const
{
    if (!sf::Joystick::isConnected(this->controller_id_) || !sf::Joystick::hasAxis(this->controller_id_, static_cast<sf::Joystick::Axis>(axis_index))) {
        return 0.0f;
    }

    const float raw_value = sf::Joystick::getAxisPosition(this->controller_id_, static_cast<sf::Joystick::Axis>(axis_index));
    float normalized_value = std::clamp(raw_value / 100.0f, -1.0f, 1.0f);

    // Apply inversion based on axis type
    if (axis_index == settings::current::gamepad_steering_axis && settings::current::gamepad_invert_steering) {
        normalized_value = -normalized_value;
    }
    else if (axis_index == settings::current::gamepad_throttle_axis && settings::current::gamepad_invert_throttle) {
        normalized_value = -normalized_value;
    }

    return apply_deadzone(normalized_value);
}

bool Gamepad::is_button_pressed(const int button_index) const
{
    if (!sf::Joystick::isConnected(this->controller_id_) || button_index < 0 || button_index >= static_cast<int>(sf::Joystick::getButtonCount(this->controller_id_))) {
        return false;
    }

    return sf::Joystick::isButtonPressed(this->controller_id_, static_cast<unsigned>(button_index));
}

bool Gamepad::is_usable() const
{
    return this->cached_info_.connected && this->cached_info_.has_configured_steering_axis && this->cached_info_.has_configured_throttle_axis && this->cached_info_.has_configured_handbrake_button;
}

float Gamepad::apply_deadzone(const float value, const float deadzone)
{
    return (std::abs(value) < deadzone) ? 0.0f : value;
}

}  // namespace core::gamepad
