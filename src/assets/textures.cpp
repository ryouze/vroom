/**
 * @file textures.cpp
 */

#include <cstddef>    // for std::size_t
#include <format>     // for std::format
#include <stdexcept>  // for std::runtime_error, std::out_of_range
#include <string>     // for std::string
#include <utility>    // for std::move

#include <SFML/Graphics/Texture.hpp>
#include <spdlog/spdlog.h>

#include "textures.hpp"

namespace assets::textures {

void TextureManager::load(const std::string &identifier,
                          const EmbeddedTexture &embedded_texture)
{
    // SPDLOG_DEBUG("Loading texture with identifier: {}", identifier);

    // Load the texture from memory
    sf::Texture texture;
    if (!texture.loadFromMemory(embedded_texture.data, embedded_texture.size)) {
        throw std::runtime_error(std::format("Failed to load texture from memory for identifier: {}", identifier));
    }

    // Set the texture smoothing option, then store it
    texture.setSmooth(true);
    this->textures_.insert_or_assign(identifier, std::move(texture));
    SPDLOG_DEBUG("Texture '{}' loaded, exiting!", identifier);
}

const sf::Texture &TextureManager::get(const std::string &identifier) const
{
    // SPDLOG_DEBUG("Retrieving texture with identifier: {}", identifier);
    if (!this->textures_.contains(identifier)) {
        throw std::out_of_range(std::format("Texture identifier not found: {}", identifier));
    }
    SPDLOG_DEBUG("Texture '{}' found, returning it!", identifier);
    return this->textures_.at(identifier);
}

std::size_t TextureManager::size() const
{
    // SPDLOG_TRACE("Returning size of TextureManager: '{}'!", this->size_);
    return this->textures_.size();
}

}  // namespace assets::textures
