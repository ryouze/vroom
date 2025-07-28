/**
 * @file textures.test.cpp
 */

#include <string>
#include <vector>

#include <SFML/Graphics.hpp>
#include <snitch/snitch.hpp>

#include "assets/textures.hpp"

TEST_CASE("TextureManager: load and get texture", "[src][assets][textures.hpp]")
{
    assets::textures::TextureManager texture_manager;
    // Create a minimal valid PNG header (not a real image, but enough for SFML to fail gracefully)
    const unsigned char fake_png_data[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    const assets::textures::TextureManager::EmbeddedTexture embedded_texture = {fake_png_data, sizeof(fake_png_data)};
    // Should throw when loading invalid PNG data
    REQUIRE_THROWS_AS(texture_manager.load("car_black", embedded_texture), std::runtime_error);
    // Should not store anything on failure
    REQUIRE(texture_manager.size() == 0);
}

TEST_CASE("TextureManager: get throws for missing texture", "[src][assets][textures.hpp]")
{
    const assets::textures::TextureManager texture_manager;
    REQUIRE_THROWS_AS(texture_manager.get("missing"), std::out_of_range);
}

TEST_CASE("TextureManager: size returns correct count", "[src][assets][textures.hpp]")
{
    assets::textures::TextureManager texture_manager;
    REQUIRE(texture_manager.size() == 0);
    // Try to load two invalid textures
    const unsigned char fake_png_data[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    const assets::textures::TextureManager::EmbeddedTexture embedded_texture = {fake_png_data, sizeof(fake_png_data)};
    for (const std::string &id : std::vector<std::string>{"car_black", "car_white"}) {
        try {
            texture_manager.load(id, embedded_texture);
        }
        catch (const std::runtime_error &) {
        }
    }
    REQUIRE(texture_manager.size() == 0);
}
