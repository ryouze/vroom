/**
 * @file textures.hpp
 *
 * @brief Load embedded textures as RAII objects.
 */

#pragma once

#include <cstddef>           // for std::size_t
#include <initializer_list>  // for std::initializer_list
#include <vector>            // for std::vector

#include <SFML/Graphics.hpp>

// Embedded road textures
#include "data/textures/road/road_sand01.hpp"
#include "data/textures/road/road_sand35.hpp"
#include "data/textures/road/road_sand37.hpp"
#include "data/textures/road/road_sand39.hpp"
#include "data/textures/road/road_sand87.hpp"
#include "data/textures/road/road_sand88.hpp"
#include "data/textures/road/road_sand89.hpp"

// Embedded car textures
#include "data/textures/car/car_black_1.hpp"
#include "data/textures/car/car_blue_1.hpp"
#include "data/textures/car/car_green_1.hpp"
#include "data/textures/car/car_red_1.hpp"
#include "data/textures/car/car_yellow_1.hpp"

namespace assets::textures {

/**
 * @brief Class that loads and manages embedded SFML textures.
 *
 * On construction, the class loads a set of textures from memory and stores them for later retrieval.
 */
class TextureManager final {
  public:
    /**
     * @brief Parameter struct for a single embedded texture. Holds pointer to the texture data and its size.
     */
    struct EmbeddedTexture final {
        /**
         * @brief Pointer to the texture data in memory.
         */
        const unsigned char *data;

        /**
         * @brief Size (in bytes) of the texture data.
         */
        std::size_t size;
    };

    /**
     * @brief Construct a new TextureManager object.
     *
     * @param embedded_textures List of embedded textures to load from memory (e.g., "{{car::data, car::size}}").
     * @param smooth If true, enable texture smoothing, otherwise keep it disabled, which is the SFML's default behavior (default: true).
     *
     * @throws std::runtime_error if failed to load a texture from memory.
     */
    explicit TextureManager(const std::initializer_list<EmbeddedTexture> &embedded_textures,
                            const bool smooth = true);

    /**
     * @brief Retrieve a texture by index.
     *
     * @param index Index of the texture to retrieve (e.g., "4").
     *
     * @return Reference to the texture at the given index.
     *
     * @throws std::out_of_range if the index is out of range.
     */
    [[nodiscard]] const sf::Texture &operator[](const std::size_t index) const;

    /**
     * @brief Get the number (size) of stored textures.
     *
     * @return Number of textures (e.g., "4").
     */
    [[nodiscard]] std::size_t size() const;

    // Allow move semantics
    TextureManager(TextureManager &&) = default;
    TextureManager &operator=(TextureManager &&) = default;

    // Disable copy semantics
    TextureManager(const TextureManager &) = delete;
    TextureManager &operator=(const TextureManager &) = delete;

  private:
    /**
     * @brief Vector of loaded textures.
     */
    std::vector<sf::Texture> textures_;
};

/**
 * @brief Return a TextureManager instance with loaded road tile textures.
 *
 * The texture order is as follows:
 *  - 0: [┏] Top-left curve
 *  - 1: [┓] Top-right curve
 *  - 2: [┛] Bottom-right curve
 *  - 3: [┗] Bottom-left curve
 *  - 4: [┃] Vertical road
 *  - 5: [━] Horizontal road
 *  - 6: [━] Horizontal finish line
 *
 * @return TextureManager instance with the loaded textures. The textures are always 128x128 pixels.
 *
 * @note This should only be called once, preferably at the start of the program.
 *
 * @details The textures are loaded from embedded header files and managed with RAII. Use indexing to access them.
 */
[[nodiscard]] inline TextureManager get_road_textures()
{
    return TextureManager(  // Always 128x128px
        {
            {road_sand89::data, road_sand89::size},  // Top-left curve
            {road_sand01::data, road_sand01::size},  // Top-right curve
            {road_sand37::data, road_sand37::size},  // Bottom-right curve
            {road_sand35::data, road_sand35::size},  // Bottom-left curve
            {road_sand87::data, road_sand87::size},  // Vertical road
            {road_sand88::data, road_sand88::size},  // Horizontal road
            {road_sand39::data, road_sand39::size},  // Horizontal finish line
        });
}

/**
 * @brief Return a TextureManager instance with loaded car textures.
 *
 * The texture order is as follows:
 * - 0: Black
 * - 1: Blue
 * - 2: Green
 * - 3: Red
 * - 4: Yellow
 *
 * @return TextureManager instance with the loaded textures. The textures are usually 71x131 pixels.
 *
 * @note This should only be called once, preferably at the start of the program.
 *
 * @details The textures are loaded from embedded header files and managed with RAII. Use indexing to access them.
 */
[[nodiscard]] inline TextureManager get_car_textures()
{
    return TextureManager(  // Usually 71x131px, with the car facing right
        {
            {car_black_1::data, car_black_1::size},    // Black
            {car_blue_1::data, car_blue_1::size},      // Blue
            {car_green_1::data, car_green_1::size},    // Green
            {car_red_1::data, car_red_1::size},        // Red
            {car_yellow_1::data, car_yellow_1::size},  // Yellow
        });
}

}  // namespace assets::textures
