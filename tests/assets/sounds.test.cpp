/**
 * @file sounds.test.cpp
 */

#include <snitch/snitch.hpp>

#include "assets/sounds.hpp"

TEST_CASE("EmbeddedSound struct can be created", "[src][assets][sounds.hpp]")
{
    // Test that we can create the embedded sound struct
    const unsigned char dummy_data[] = {0x52, 0x49, 0x46, 0x46};  // RIFF header bytes
    const assets::sounds::SoundManager::EmbeddedSound sound{
        .data = dummy_data,
        .size = sizeof(dummy_data)};

    CHECK(sound.data != nullptr);
    CHECK(sound.size == sizeof(dummy_data));
}
