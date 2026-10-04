/**
 * @file textures.test.cpp
 */

#include <array> // for std::array

#include <snitch/snitch.hpp>

#include "assets/textures.hpp"

TEST_CASE("EmbeddedTexture struct can be created", "[src][assets][textures.hpp]")
{
    const std::array<unsigned char, 4> dummy_data{0x89, 0x50, 0x4E, 0x47}; // PNG header bytes
    const assets::textures::TextureManager::EmbeddedTexture texture{
        .data = dummy_data.data(),
        .size = dummy_data.size(),
    };

    CHECK(texture.data != nullptr);
    CHECK(texture.size == sizeof(dummy_data));
}
