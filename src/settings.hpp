/**
 * @file settings.hpp
 *
 * @brief Shared settings and configuration for the project.
 */

#pragma once

namespace settings {

namespace constants {

/**
 * @brief Available FPS limit labels for display in UI.
 */
inline constexpr const char *fps_labels[] = {"30", "60", "90", "120", "144", "165", "240", "360", "Unlimited"};

/**
 * @brief Available FPS limit values.
 *
 * @note A value of "0" means "unlimited" - the game will run as fast as possible, which will drain the battery quickly on mobile devices.
 */
inline constexpr unsigned fps_values[] = {30, 60, 90, 120, 144, 165, 240, 360, 0};

/**
 * @brief Available anti-aliasing level labels for display in UI.
 */
inline constexpr const char *anti_aliasing_labels[] = {"Off", "2x", "4x", "8x", "16x"};

/**
 * @brief Available anti-aliasing level values.
 */
inline constexpr unsigned anti_aliasing_values[] = {0, 2, 4, 8, 16};

/**
 * @brief Gamepad axis names for display in UI.
 */
inline constexpr const char *gamepad_axis_labels[] = {
    "X (Left Stick Left/Right)",   // 0
    "Y (Left Stick Up/Down)",      // 1
    "Z (Right Stick Left/Right)",  // 2
    "R (Right Stick Up/Down)",     // 3
    "U (Left Trigger)",            // 4
    "V (Right Trigger)",           // 5
    "PovX (D-Pad Left/Right)",     // 6
    "PovY (D-Pad Up/Down)"         // 7
};

/**
 * @brief Default width in pixels.
 *
 * @note This is only applicable in windowed mode.
 */
inline constexpr unsigned windowed_width = 1280;

/**
 * @brief Default height in pixels.
 *
 * @note This is only applicable in windowed mode.
 */
inline constexpr unsigned windowed_height = 720;

/**
 * @brief Minimum width in pixels.
 *
 * @note This is only applicable in windowed mode.
 */
inline constexpr unsigned windowed_min_width = 800;

/**
 * @brief Minimum height in pixels.
 *
 * @note This is only applicable in windowed mode.
 */
inline constexpr unsigned windowed_min_height = 600;

}  // namespace constants

namespace current {

// Set mutable settings that can be modified at runtime by a config file or the user via the GUI

/**
 * @brief Whether the game runs in fullscreen or windowed mode.
 *
 * If true, use fullscreen mode, if false, use windowed mode.
 *
 * @note This defaults to fullscreen.
 */
inline bool fullscreen = true;

/**
 * @brief Whether the game uses vertical sync (VSync).
 *
 * If true, VSync is enabled, otherwise it is disabled.
 *
 * @note This defaults to disabled.
 */
inline bool vsync = false;

/**
 * @brief Frame per second (FPS) limit index for the game.
 *
 * If vsync is enabled, this value is ignored. This is an index into the "fps_values" array.
 *
 * @note This defaults to 144 FPS (index 4).
 */
inline int fps_idx = 4;

/**
 * @brief Index of the fullscreen resolution.
 *
 * If the game is in windowed mode, this value is ignored.
 *
 * @note This defaults to the best available resolution (0).
 */
inline int mode_idx = 0;

/**
 * @brief Anti-aliasing level index.
 *
 * This is an index into the "anti_aliasing_values" array.
 *
 * @note This defaults to 8x anti-aliasing (index 3).
 */
inline int anti_aliasing_idx = 3;

/**
 * @brief Whether tire marks are enabled for the selected car.
 *
 * If true, tire marks will be displayed when drifting. If false, tire marks are disabled for better performance.
 *
 * @note This defaults to enabled.
 */
inline bool tire_marks = true;

/**
 * @brief Whether to prefer gamepad input over keyboard when both are available.
 *
 * If true, gamepad input will be used when available. If false, keyboard input will be used even if gamepad is connected.
 *
 * @note This defaults to preferring gamepad input.
 */
inline bool prefer_gamepad = true;

/**
 * @brief Gamepad axis used for steering (left/right).
 *
 * This corresponds to SFML sf::Joystick::Axis enum values.
 *
 * @note This defaults to X axis (0) which works for Xbox controllers on macOS.
 */
inline int gamepad_steering_axis = 0;  // sf::Joystick::Axis::X

/**
 * @brief Gamepad axis used for gas/throttle.
 *
 * This corresponds to SFML sf::Joystick::Axis enum values.
 *
 * @note This defaults to R axis (3) which is right stick up/down and works on macOS.
 */
inline int gamepad_gas_axis = 3;  // sf::Joystick::Axis::R

/**
 * @brief Gamepad axis used for brake.
 *
 * This corresponds to SFML sf::Joystick::Axis enum values.
 *
 * @note This defaults to R axis (3) which is right stick up/down and works on macOS.
 */
inline int gamepad_brake_axis = 3;  // sf::Joystick::Axis::R

/**
 * @brief Gamepad button used for handbrake.
 *
 * This is an index into the SFML joystick buttons.
 *
 * @note This defaults to button 0 which is typically the A button on Xbox controllers.
 */
inline int gamepad_handbrake_button = 0;

/**
 * @brief Whether to invert the steering axis.
 *
 * If true, the steering axis will be inverted (left becomes right and vice versa).
 *
 * @note This defaults to false.
 */
inline bool gamepad_invert_steering = false;

/**
 * @brief Whether to invert the gas axis.
 *
 * If true, the gas axis will be inverted.
 *
 * @note This defaults to false.
 */
inline bool gamepad_invert_gas = false;

/**
 * @brief Whether to invert the brake axis.
 *
 * If true, the brake axis will be inverted.
 *
 * @note This defaults to false.
 */
inline bool gamepad_invert_brake = false;

/**
 * @brief Engine sound volume as a float (0.0-1.0).
 *
 * Controls the volume of the engine sound effect.
 *
 * @note This defaults to 0.4 (40%).
 */
inline float engine_volume = 0.4f;

/**
 * @brief Tire screeching sound volume as a float (0.0-1.0).
 *
 * Controls the volume of the tire screeching sound effect when drifting.
 *
 * @note This defaults to 0.6 (60%).
 */
inline float tire_screech_volume = 0.6f;

/**
 * @brief Wall hit sound volume as a float (0.0-1.0).
 *
 * Controls the volume of the wall hit sound effect when the car collides with track boundaries.
 *
 * @note This defaults to 0.60 (60%).
 */
inline float wall_hit_volume = 0.60f;

/**
 * @brief UI sound volume as a float (0.0-1.0).
 *
 * Controls the volume of the UI sound effects (buttons, menus, etc.).
 *
 * @note This defaults to 0.7 (70%).
 */
inline float ui_volume = 0.7f;

}  // namespace current

}  // namespace settings
