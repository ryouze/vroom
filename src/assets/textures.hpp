/**
 * @file textures.hpp
 *
 * @brief Load and manage embedded SFML textures.
 */

#pragma once

#include <cstddef>        // for std::size_t
#include <string>         // for std::string
#include <unordered_map>  // for std::unordered_map

#include <SFML/Graphics.hpp>

namespace assets::textures {

/**
 * @brief Class that loads and manages embedded SFML textures.
 *
 * On construction, the class does nothing. Use the "load()" method to load textures from memory.
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
     * @brief Default constructor.
     */
    TextureManager() = default;

    /**
     * @brief Load a texture from memory and store it at the given identifier.
     *
     * @param identifier Unique identifier for the texture (e.g., "car_black").
     * @param embedded_texture Embedded texture data to load from memory.
     *
     * @throws std::runtime_error if failed to load the texture from memory.
     *
     * @note If the identifier already exists, the previous texture is overwritten, mirroring "operator[]" on the map.
     */
    void load(const std::string &identifier,
              const EmbeddedTexture &embedded_texture);

    /**
     * @brief Get a texture by its identifier.
     *
     * @param identifier Unique identifier for the texture (e.g., "car_black").
     *
     * @return Const reference to the texture.
     *
     * @throws std::out_of_range if the identifier is not found.
     */
    [[nodiscard]] const sf::Texture &get(const std::string &identifier) const;

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
     * @brief Map of loaded textures, where the key is a unique identifier (e.g., "car_black") and the value is the loaded texture.
     */
    std::unordered_map<std::string, sf::Texture> textures_;
};

}  // namespace assets::textures
