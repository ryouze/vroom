/**
 * @file gamepad.cpp
 */

#include <algorithm>  // for std::clamp
#include <cstdlib>    // for std::abs
#include <string>     // for std::string

#include <SFML/Window/Joystick.hpp>
#include <spdlog/spdlog.h>

#include "gamepad.hpp"

namespace core::gamepad {

namespace {

/**
 * @brief Apply deadzone to joystick input to prevent small movements from being registered.
 *
 * @param value Value to apply deadzone to (e.g., joystick axis value).
 * @param threshold Deadzone threshold (default: "0.1f").
 *
 * @return Value but with deadzone applied. If the absolute value is less than the threshold, it returns 0.0f; otherwise, it returns the original value.
 */
[[nodiscard]] float apply_deadzone(const float value,
                                   const float threshold = 0.1f)
{
    // Apply deadzone to joystick input
    return (std::abs(value) < threshold) ? 0.0f : value;
}

}  // namespace

[[nodiscard]] bool is_valid(const unsigned id)
{
    // Controller support: Xbox layout (SFML Joystick 0)
    // SFML Docs: Joystick states are automatically updated when you check for events. If you don't check for events, or need to query a joystick state (for example, checking which joysticks are connected) before starting your game loop, you'll have to manually call the sf::Joystick::update() function yourself to make sure that the joystick states are up to date.
    // Reference: https://www.sfml-dev.org/tutorials/3.0/window/inputs/#joystick

    if (sf::Joystick::isConnected(id)) {
        // SPDLOG_DEBUG("Controller with ID '{}' is connected: '{}'", id, sf::Joystick::getIdentification(id).name.toAnsiString());

        // Check if buttons are available
        const unsigned int button_count = sf::Joystick::getButtonCount(id);
        // if (button_count == 0) {
        //     SPDLOG_WARN("Controller has no buttons!");
        //     return false;
        // }
        // Check if at least 11 buttons are available (11th is used for pause)
        if (button_count < 11) [[unlikely]] {
            SPDLOG_WARN("Controller has only {} buttons, but at least 11 are required!", button_count);
            return false;
        }

        // SPDLOG_DEBUG("Controller has '{}' buttons", button_count);

        // Check if joystick has all the required axes
        // X - steer left/right
        if (!sf::Joystick::hasAxis(id, sf::Joystick::Axis::X)) [[unlikely]] {
            // If no Z axis, skip
            SPDLOG_WARN("Controller does not have an X axis!");
            return false;
        }
        // R - gas/brake
        if (!sf::Joystick::hasAxis(id, sf::Joystick::Axis::R)) [[unlikely]] {
            // If no Z axis, skip
            SPDLOG_WARN("Controller does not have a R axis!");
            return false;
        }

        return true;
    }

    // SPDLOG_WARN("Controller with ID '{}' is not connected!", id);
    return false;
}

std::string get_name(const unsigned id)
{
    const std::string name = sf::Joystick::getIdentification(id).name.toAnsiString();
    return name.empty() ? "Generic Controller" : name;  // Fallback to a generic name if empty
}

[[nodiscard]] float get_steer(const unsigned id)
{
    return apply_deadzone(std::clamp(
        sf::Joystick::getAxisPosition(id, sf::Joystick::Axis::X) / 100.0f,
        -1.0f, 1.0f));  // [-1.0, 1.0]
}

[[nodiscard]] float get_throttle(const unsigned id)
{
    return apply_deadzone(std::clamp(
        -sf::Joystick::getAxisPosition(id, sf::Joystick::Axis::R) / 100.0f,
        -1.0f, 1.0f));  // [-1.0, 1.0], flipped so 1 = gas, -1 = brake
}

[[nodiscard]] bool get_handbrake(const unsigned id)
{
    return sf::Joystick::isButtonPressed(id, 0);  // A button
}

// [[nodiscard]] bool get_pause(const unsigned id)
// {
//     return sf::Joystick::isButtonPressed(id, 11);  // Start (?) button
// }

// [[nodiscard]] bool get_confirm(const unsigned id)
// {
//     return sf::Joystick::isButtonPressed(id, 0);  // A button
// }

// [[nodiscard]] bool get_cancel(const unsigned id)
// {
//     return sf::Joystick::isButtonPressed(id, 1);  // B button
// }

}  // namespace core::gamepad
