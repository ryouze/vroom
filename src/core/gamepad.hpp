/**
 * @file gamepad.hpp
 *
 * @brief Gamepad input handling with configurable axis mapping.
 */

#pragma once

#include <string>  // for std::string
#include <vector>  // for std::vector

namespace core::gamepad {

/**
 * @brief Information about a connected gamepad.
 */
struct GamepadInfo {
    /**
     * @brief Whether the gamepad is connected.
     */
    bool connected = false;

    /**
     * @brief Name of the gamepad.
     */
    std::string name = "No controller";

    /**
     * @brief Number of buttons available on the gamepad.
     */
    unsigned button_count = 0;

    /**
     * @brief List of available axis indices (0-7).
     */
    std::vector<int> available_axes;

    /**
     * @brief Whether the currently configured steering axis is available.
     */
    bool has_configured_steering_axis = false;

    /**
     * @brief Whether the currently configured throttle axis is available.
     */
    bool has_configured_throttle_axis = false;

    /**
     * @brief Whether the currently configured handbrake button is available.
     */
    bool has_configured_handbrake_button = false;
};

/**
 * @brief Input values from the gamepad.
 */
struct GamepadInput {
    /**
     * @brief Steering value (-1.0 to 1.0, left to right).
     */
    float steering = 0.0f;

    /**
     * @brief Throttle value (0.0 to 1.0).
     */
    float throttle = 0.0f;

    /**
     * @brief Brake value (0.0 to 1.0).
     */
    float brake = 0.0f;

    /**
     * @brief Whether handbrake is pressed.
     */
    bool handbrake = false;
};

/**
 * @brief Gamepad manager class for handling controller input.
 */
class Gamepad {
  public:
    /**
     * @brief Create a gamepad manager for the specified controller ID.
     *
     * @param controller_id ID of the controller to manage (default: 0).
     */
    explicit Gamepad(unsigned controller_id = 0);

    /**
     * @brief Get information about the current gamepad.
     *
     * @return GamepadInfo struct containing connection status, name, and capabilities.
     */
    [[nodiscard]] GamepadInfo get_info() const;

    /**
     * @brief Get current input from the gamepad.
     *
     * @return GamepadInput struct with all current input values.
     */
    [[nodiscard]] GamepadInput get_input() const;

    /**
     * @brief Get raw axis value for display purposes.
     *
     * @param axis_index Axis index (0-7).
     *
     * @return Raw axis value (-100.0 to 100.0) or 0.0 if axis not available.
     */
    [[nodiscard]] float get_raw_axis_value(int axis_index) const;

    /**
     * @brief Check if a specific button is pressed.
     *
     * @param button_index Button index.
     *
     * @return True if button is pressed, false otherwise.
     */
    [[nodiscard]] bool is_button_pressed(int button_index) const;

    /**
     * @brief Check if the gamepad is usable with current configuration.
     *
     * @return True if gamepad is connected and has all configured axes/buttons.
     */
    [[nodiscard]] bool is_usable() const;

  private:
    /**
     * @brief Controller ID to manage.
     */
    unsigned controller_id_;

    /**
     * @brief Apply deadzone to an input value.
     *
     * @param value Input value.
     * @param deadzone Deadzone threshold (default: 0.1f).
     *
     * @return Value with deadzone applied.
     */
    [[nodiscard]] static float apply_deadzone(float value, float deadzone = 0.1f);
};

}  // namespace core::gamepad
