/**
 * @file sounds.hpp
 *
 * @brief Load and manage embedded SFML sounds.
 */

#pragma once

#include <cstddef>        // for std::size_t
#include <string>         // for std::string
#include <unordered_map>  // for std::unordered_map

#include <SFML/Audio.hpp>

namespace assets::sounds {

/**
 * @brief Class that loads and manages embedded SFML sound buffers.
 *
 * On construction, the class does nothing. Use the "load()" method to load sound buffers from memory.
 */
class SoundManager final {
  public:
    /**
     * @brief Parameter struct for a single embedded sound. Holds pointer to the sound data and its size.
     */
    struct EmbeddedSound final {
        /**
         * @brief Pointer to the sound data in memory.
         */
        const unsigned char *data;

        /**
         * @brief Size (in bytes) of the sound data.
         */
        std::size_t size;
    };

    /**
     * @brief Default constructor.
     */
    SoundManager() = default;

    /**
     * @brief Load a sound buffer from memory and store it at the given identifier.
     *
     * @param identifier Unique identifier for the sound buffer (e.g., "engine").
     * @param embedded_sound Embedded sound data to load from memory.
     *
     * @throws std::runtime_error if failed to load the sound buffer from memory.
     *
     * @note If the identifier already exists, the previous sound buffer is overwritten, mirroring "operator[]" on the map.
     */
    void load(const std::string &identifier,
              const EmbeddedSound &embedded_sound);

    /**
     * @brief Get a sound buffer by its identifier.
     *
     * @param identifier Unique identifier for the sound buffer (e.g., "engine").
     *
     * @return Const reference to the sound buffer.
     *
     * @throws std::out_of_range if the identifier is not found.
     */
    [[nodiscard]] const sf::SoundBuffer &get(const std::string &identifier) const;

    /**
     * @brief Get the number (size) of stored sound buffers.
     *
     * @return Number of sound buffers (e.g., "4").
     */
    [[nodiscard]] std::size_t size() const;

    // Allow move semantics
    SoundManager(SoundManager &&) = default;
    SoundManager &operator=(SoundManager &&) = default;

    // Disable copy semantics
    SoundManager(const SoundManager &) = delete;
    SoundManager &operator=(const SoundManager &) = delete;

  private:
    /**
     * @brief Map of loaded sound buffers, where the key is a unique identifier (e.g., "engine") and the value is the loaded sound buffer.
     */
    std::unordered_map<std::string, sf::SoundBuffer> sound_buffers_;
};

}  // namespace assets::sounds
