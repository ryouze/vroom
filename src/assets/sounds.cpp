/**
 * @file sounds.cpp
 */

#include <cstddef>    // for std::size_t
#include <format>     // for std::format
#include <stdexcept>  // for std::runtime_error, std::out_of_range
#include <string>     // for std::string
#include <utility>    // for std::move

#include <SFML/Audio.hpp>
#include <spdlog/spdlog.h>

#include "sounds.hpp"

namespace assets::sounds {

void SoundManager::load(const std::string &identifier,
                        const EmbeddedSound &embedded_sound)
{
    // SPDLOG_DEBUG("Loading sound buffer with identifier: {}", identifier);

    // Load the sound buffer from memory
    sf::SoundBuffer sound_buffer;
    if (!sound_buffer.loadFromMemory(embedded_sound.data, embedded_sound.size)) {
        throw std::runtime_error(std::format("Failed to load sound buffer from memory for identifier: {}", identifier));
    }

    // Store the sound buffer
    this->sound_buffers_.insert_or_assign(identifier, std::move(sound_buffer));
    SPDLOG_DEBUG("Sound buffer '{}' loaded, exiting!", identifier);
}

const sf::SoundBuffer &SoundManager::get(const std::string &identifier) const
{
    // SPDLOG_DEBUG("Retrieving sound buffer with identifier: {}", identifier);
    if (!this->sound_buffers_.contains(identifier)) {
        throw std::out_of_range(std::format("Sound buffer identifier not found: {}", identifier));
    }
    SPDLOG_DEBUG("Sound buffer '{}' found, returning it!", identifier);
    return this->sound_buffers_.at(identifier);
}

std::size_t SoundManager::size() const
{
    // SPDLOG_TRACE("Returning size of SoundManager: '{}'!", this->size_);
    return this->sound_buffers_.size();
}

}  // namespace assets::sounds
