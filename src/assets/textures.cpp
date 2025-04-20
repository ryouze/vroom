/**
 * @file manager.cpp
 */

#include <cstddef>           // for std::size_t
#include <format>            // for std::format
#include <initializer_list>  // for std::initializer_list
#include <stdexcept>         // for std::runtime_error, std::out_of_range
#include <utility>           // for std::move

#include <SFML/Graphics.hpp>
#include <spdlog/spdlog.h>

#include "textures.hpp"

namespace assets::textures {

TextureManager::TextureManager(const std::initializer_list<EmbeddedTexture> &embedded_textures,
                               const bool smooth)
{
    // Get the number of textures to be loaded; can't do this at compile time unfortunately
    const std::size_t size = embedded_textures.size();

    SPDLOG_DEBUG("Creating TextureManager with '{}' textures and smooth='{}'...", size, smooth);
#ifndef NDEBUG  // Skip this runtime check in release builds
    if (size == 0) [[unlikely]] {
        throw std::runtime_error("TextureManager cannot be created with zero textures");
    }
#endif

    this->textures_.reserve(size);

    // Track the index of the texture being loaded for exception messages
    // This is not strictly necessary, but it can help with debugging
    std::size_t idx = 0;

    // Load each texture individually
    for (const auto &input_texture : embedded_textures) {
        // Load the texture from memory
        sf::Texture texture;
        if (!texture.loadFromMemory(input_texture.data, input_texture.size)) [[unlikely]] {
            throw std::runtime_error(std::format("Failed to load texture from memory at index '{}' (total={})", idx, size));
        }

        // Set the texture smoothing option, then append it to the vector
        texture.setSmooth(smooth);
        this->textures_.emplace_back(std::move(texture));
        ++idx;
    }

    // Potentially unnecessary, since we reserved the size beforehand, but we're not gonna add more textures
    this->textures_.shrink_to_fit();

    SPDLOG_DEBUG("All '{}' textures were loaded successfully, exiting constructor!", this->textures_.size());
}

const sf::Texture &TextureManager::operator[](const std::size_t index) const
{
    if (const std::size_t size = this->textures_.size();
        index >= size) [[unlikely]] {
        throw std::out_of_range(std::format("Requested texture index '{}' is out of range (size={})", index, size));
    }
    // SPDLOG_TRACE("Returning texture reference at index '{}'!", index);
    return this->textures_[index];
}

std::size_t TextureManager::size() const
{
    // SPDLOG_TRACE("Returning size of TextureManager: '{}'!", this->size_);
    return this->textures_.size();
}

}  // namespace assets::textures
