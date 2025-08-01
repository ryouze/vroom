/**
 * @file settings.hpp
 *
 * @brief Shared settings and configuration for the project.
 */

#pragma once

namespace settings {

namespace defaults {

// NOTE: This is not implemented yet. The goal is to use these values throught the entire codebase.

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
 * @brief Frame per second (FPS) limit for the game.
 *
 * If vsync is enabled, this value is ignored.
 *
 * @note This defaults to 144 FPS (4).
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

}  // namespace defaults

namespace current {

// Set to default values, but they will be overwritten by the config file or by user

inline bool fullscreen = defaults::fullscreen;
inline bool vsync = defaults::vsync;
inline int fps_idx = defaults::fps_idx;
inline int resolution_idx = defaults::resolution_idx;

}  // namespace current

}  // namespace settings
