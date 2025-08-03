/**
 * @file settings.hpp
 *
 * @brief Shared settings and configuration for the project.
 */

#pragma once

namespace settings {

namespace defaults {

/**
 * @brief Whether the game runs in fullscreen or windowed mode.
 *
 * If true, use fullscreen mode, otherwise use windowed mode.
 *
 * @note This defaults to fullscreen.
 */
inline constexpr bool fullscreen = true;

/**
 * @brief Whether the game uses vertical sync (VSync).
 *
 * If true, VSync is enabled, otherwise it is disabled.
 *
 * @note This defaults to disabled.
 */
inline constexpr bool vsync = false;

/**
 * @brief Frame per second (FPS) limit index for the game.
 *
 * If vsync is enabled, this value is ignored. This is an index into the fps_values array.
 *
 * @note This defaults to 144 FPS (index 4).
 */
inline constexpr int fps_idx = 4;

/**
 * @brief Index of the fullscreen resolution.
 *
 * If the game is in windowed mode, this value is ignored.
 *
 * @note This defaults to the best available resolution (0).
 */
inline constexpr int resolution_idx = 0;

/**
 * @brief Default windowed resolution width in pixels.
 */
inline constexpr unsigned windowed_width = 1280;

/**
 * @brief Default windowed resolution height in pixels.
 */
inline constexpr unsigned windowed_height = 720;

/**
 * @brief Minimum window width in pixels.
 */
inline constexpr unsigned minimum_width = 800;

/**
 * @brief Minimum window height in pixels.
 */
inline constexpr unsigned minimum_height = 600;

/**
 * @brief Anti-aliasing level.
 *
 * @note This defaults to 8x anti-aliasing.
 */
inline constexpr unsigned anti_aliasing_level = 8;

}  // namespace defaults

namespace fps {

/**
 * @brief Available FPS limit labels for display in UI.
 */
inline constexpr const char *labels[] = {"30", "60", "90", "120", "144", "165", "240", "360", "Unlimited"};

/**
 * @brief Available FPS limit values (0 means unlimited).
 */
inline constexpr unsigned values[] = {30, 60, 90, 120, 144, 165, 240, 360, 0};

}  // namespace fps

namespace current {

// Set to default values, but they will be overwritten by the config file or by user

inline bool fullscreen = defaults::fullscreen;
inline bool vsync = defaults::vsync;
inline int fps_idx = defaults::fps_idx;
inline int resolution_idx = defaults::resolution_idx;

}  // namespace current

}  // namespace settings
