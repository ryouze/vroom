/**
 * @file manager.cpp
 */

#include <cstddef>           // for std::size_t
#include <format>            // for std::format
#include <initializer_list>  // for std::initializer_list
#include <memory>            // for std::make_unique
#include <stdexcept>         // for std::runtime_error, std::out_of_range

#include <SFML/Graphics.hpp>
#include <spdlog/spdlog.h>

#include "textures.hpp"

namespace assets::textures {

TextureManager::TextureManager(const std::initializer_list<EmbeddedTexture> &embedded_textures,
                               const bool smooth)
    : size_(embedded_textures.size())
{
    SPDLOG_DEBUG("Creating TextureManager with '{}' textures and smooth='{}'...", this->size_, smooth);
    if (this->size_ == 0) [[unlikely]] {
        throw std::runtime_error("TextureManager cannot be created with zero textures");
    }

    // Create a unique pointer array to hold the textures
    // A std::array would be cleaner, but when I tried it, the user-facing API was really ugly and verbose :/
    this->textures_ = std::make_unique<sf::Texture[]>(this->size_);
    SPDLOG_DEBUG("Created pointer array, loading textures...");

    // Track the index of the texture being loaded for exception messages
    // This is not strictly necessary, but it can help with debugging
    std::size_t idx = 0;

    // Load each texture individually
    for (const auto &input_texture : embedded_textures) {
        // Load the texture from memory
        if (!this->textures_[idx].loadFromMemory(input_texture.data, input_texture.size)) [[unlikely]] {
            throw std::runtime_error(std::format("Failed to load texture from memory at index '{}' (total={})", idx, this->size_));
        }

        // Set the texture smoothing option and increment the index
        this->textures_[idx].setSmooth(smooth);
        ++idx;

        // SPDLOG_TRACE("Texture '{}' out of '{}' loaded!", idx, this->size_);
    }

    SPDLOG_DEBUG("All '{}' textures were loaded successfully, exiting constructor!", this->size_);
}

const sf::Texture &TextureManager::operator[](const std::size_t index) const
{
    if (index >= this->size_) [[unlikely]] {
        throw std::out_of_range(std::format("Requested texture index '{}' is out of range (size={})", index, this->size_));
    }
    // SPDLOG_TRACE("Returning texture reference at index '{}'!", index);
    return this->textures_[index];
}

std::size_t TextureManager::size() const
{
    // SPDLOG_TRACE("Returning size of TextureManager: '{}'!", this->size_);
    return this->size_;
}

}  // namespace assets::textures
