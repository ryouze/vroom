/**
 * @file builder.hpp
 *
 * @brief Provides functions to construct TextureManager and SoundManager instances, preloaded with all embedded game assets.
 *
 * This header centralizes asset management for the game, offering a higher-level interface compared to `sounds.hpp` and `textures.hpp`.
 */

#pragma once

#include "sounds.hpp"
#include "textures.hpp"

namespace assets::builder {

/**
 * @brief Build and return a TextureManager populated with all embedded textures.
 *
 * Available texture identifiers:
 * - Road textures: "top_left", "top_right", "bottom_right", "bottom_left", "vertical", "horizontal", "horizontal_finish"
 * - Car textures: "car_black", "car_blue", "car_green", "car_red", "car_yellow"
 *
 * @return TextureManager instance with all game textures loaded and ready to use.
 */
[[nodiscard]] assets::textures::TextureManager build_texture_manager();

/**
 * @brief Build and return a SoundManager populated with all embedded sounds.
 *
 * Available sound identifiers:
 * - Car sounds: "engine", "tires", "hit"
 * - UI sounds: "ok", "other"
 *
 * @return SoundManager instance with all game audio loaded and ready to use.
 */
[[nodiscard]] assets::sounds::SoundManager build_sound_manager();

}  // namespace assets::builder
