/**
 * @file gamepad.hpp
 *
 * @brief Xbox gamepad input handling.
 */

#pragma once

#include <string>  // for std::string

namespace core::gamepad {

/**
 * @brief Check if a gamepad is connected and has the required functionality.
 *
 * @param id ID of the gamepad to check (default: "0").
 *
 * @return True if the gamepad is valid and usable, false otherwise.
 *
 * @note Always run this function before using any other gamepad functions to ensure the controller is connected and has the required axes and buttons.
 */
[[nodiscard]] bool is_valid(const unsigned id = 0);

/**
 * @brief Get the name of the controller.
 *
 * @param id ID of the gamepad to check (default: "0").
 *
 * @return Name of the controller as a string. If not available, "Generic Controller" is returned instead.
 */
[[nodiscard]] std::string get_name(const unsigned id = 0);

/**
 * @brief Get the steering input from the controller.
 *
 * @param id ID of the gamepad to check (default: "0").
 *
 * @return Value between -1.0 (full left) and 1.0 (full right) representing the steering input.
 *
 * @note Deadzone is applied before returning the value, so small movements will not be registered.
 */
[[nodiscard]] float get_steer(const unsigned id = 0);

/**
 * @brief Get the throttle/brake input from the controller.
 *
 * @param id ID of the gamepad to check (default: "0").
 *
 * @return Value between -1.0 (full brake) and 1.0 (full throttle) representing the throttle/brake input.
 *
 * @note Deadzone is applied before returning the value, so small movements will not be registered.
 *
 * @details The low-level output is flipped, so that 1.0 represents full throttle and -1.0 represents full brake, instead of the other way around.
 */
[[nodiscard]] float get_throttle(const unsigned id = 0);

/**
 * @brief Check if the handbrake button is pressed.
 *
 * @param id ID of the gamepad to check (default: "0").
 *
 * @return True if the handbrake button is pressed, false otherwise.
 */
[[nodiscard]] bool get_handbrake(const unsigned id = 0);

// /**
//  * @brief Check if the pause button is pressed.
//  *
//  * @param id ID of the gamepad to check (default: "0").
//  *
//  * @return True if the pause button is pressed, false otherwise.
//  */
// [[nodiscard]] bool get_pause(const unsigned id = 0);

// /**
//  * @brief Check if the confirm button is pressed.
//  *
//  * @param id ID of the gamepad to check (default: "0").
//  *
//  * @return True if the confirm button is pressed, false otherwise.
//  */
// [[nodiscard]] bool get_confirm(const unsigned id = 0);

// /**
//  * @brief Check if the cancel button is pressed.
//  *
//  * @param id ID of the gamepad to check (default: "0").
//  *
//  * @return True if the cancel button is pressed, false otherwise.
//  */
// [[nodiscard]] bool get_cancel(const unsigned id = 0);

}  // namespace core::gamepad
