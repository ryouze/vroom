/**
 * @file sounds.test.cpp
 */

#include <array> // for std::array

#include <snitch/snitch.hpp>

#include "assets/sounds.hpp"

TEST_CASE("EmbeddedSound struct can be created", "[src][assets][sounds.hpp]")
{
    const std::array<unsigned char, 4> dummy_data{0x52, 0x49, 0x46, 0x46}; // RIFF header bytes
    const assets::sounds::SoundManager::EmbeddedSound sound{
        .data = dummy_data.data(),
        .size = dummy_data.size(),
    };

    CHECK(sound.data != nullptr);
    CHECK(sound.size == sizeof(dummy_data));
}
