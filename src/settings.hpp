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
 * @note This defaults to 144 FPS.
 */
inline constexpr int fps_limit = 144;

/**
 * @brief Index of the fullscreen resolution.
 *
 * If the game is in windowed mode, this value is ignored.
 *
 * @note This defaults to the best available resolution (0).
 */
inline constexpr int resolution_idx = 0;

}  // namespace defaults

}  // namespace settings
