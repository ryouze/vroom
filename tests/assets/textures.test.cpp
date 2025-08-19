/**
 * @file textures.test.cpp
 */

#include <snitch/snitch.hpp>

#include "assets/textures.hpp"

TEST_CASE("EmbeddedTexture struct can be created", "[src][assets][textures.hpp]")
{
    // Test that we can create the embedded texture struct
    const unsigned char dummy_data[] = {0x89, 0x50, 0x4E, 0x47};  // PNG header bytes
    const assets::textures::TextureManager::EmbeddedTexture texture{
        .data = dummy_data,
        .size = sizeof(dummy_data)};

    CHECK(texture.data != nullptr);
    CHECK(texture.size == sizeof(dummy_data));
}
