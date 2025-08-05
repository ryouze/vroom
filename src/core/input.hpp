/**
 * @file input.hpp
 *
 * @brief Game input handling.
 */

#pragma once

namespace core::input {

/**
 * @brief Xbox controller abstraction with configurable axes and buttons.
 *
 * On construction, the member variables are initialized. Use the "is_connected()" method, then, if available, retrieve inputs using the provided methods. The values themselves can be modified in the "settings.hpp" file.

 * @note Always run "is_connected()" once per frame in the main game loop before using any input methods.
 */
class Gamepad {
  public:
    /**
     * @brief Construct a new Gamepad object.
     *
     * @param id Gamepad ID (default: "0").
     * @param deadzone Deadzone for all analog inputs (default: "0.15").
     */
    explicit Gamepad(const unsigned int id = 0,
                     const float deadzone = 0.15f);

    /**
     * @brief Default destructor.
     */
    ~Gamepad() = default;

    // Allow copy and move construction but disable assignment (due to const members)
    Gamepad(const Gamepad &) = default;
    Gamepad &operator=(const Gamepad &) = delete;
    Gamepad(Gamepad &&) = default;
    Gamepad &operator=(Gamepad &&) = delete;

    /**
     * @brief Check if the gamepad is currently connected.
     *
     * @return True if the gamepad is connected, false otherwise.
     */
    [[nodiscard]] bool is_connected() const;

    /**
     * @brief Get the gas input from the configured axis.
     *
     * @return Value in the [0.0, 1.0] range, where "0" is no throttle and "1" is full throttle.
     */
    [[nodiscard]] float get_gas() const;

    /**
     * @brief Get the brake input from the configured axis.
     *
     * @return Value in the [0.0, 1.0] range, where "0" is no braking and "1" is full brake.
     */
    [[nodiscard]] float get_brake() const;

    /**
     * @brief Get steering input from the configured axis.
     *
     * @return Value in the range [-1.0, 1.0] where "-1" is full left, "0" is center, and "1" is full right.
     */
    [[nodiscard]] float get_steer() const;

    /**
     * @brief Get handbrake input from the configured button.
     *
     * @return True if the button is pressed, false otherwise.
     */
    [[nodiscard]] bool get_handbrake() const;

    /**
     * @brief Get the number of available buttons on the gamepad.
     *
     * @return Number of supported buttons.
     */
    [[nodiscard]] unsigned int get_button_count() const;

  private:
    /**
     * @brief Apply deadzone to analog input.
     *
     * @param value Input value in any range.
     *
     * @return Value with deadzone applied, maintaining the original range.
     */
    [[nodiscard]] float apply_deadzone(const float value) const;

    /**
     * @brief Gamepad ID.
     */
    const unsigned int id_;

    /**
     * @brief Deadzone threshold for all analog inputs.
     */
    const float deadzone_;
};

}  // namespace core::input
