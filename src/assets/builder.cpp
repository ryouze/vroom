/**
 * @file builder.cpp
 */

// Managers
#include "sounds.hpp"
#include "textures.hpp"

// Embedded road textures
#include "data/textures/road/road_sand01.hpp"
#include "data/textures/road/road_sand35.hpp"
#include "data/textures/road/road_sand37.hpp"
#include "data/textures/road/road_sand39.hpp"
#include "data/textures/road/road_sand87.hpp"
#include "data/textures/road/road_sand88.hpp"
#include "data/textures/road/road_sand89.hpp"

// Embedded car textures
#include "data/textures/car/car_black_1.hpp"
#include "data/textures/car/car_blue_1.hpp"
#include "data/textures/car/car_green_1.hpp"
#include "data/textures/car/car_red_1.hpp"
#include "data/textures/car/car_yellow_1.hpp"

// Embedded sounds
#include "data/sounds/car/engine.hpp"
#include "data/sounds/car/hit.hpp"
#include "data/sounds/car/tires.hpp"
#include "data/sounds/ui/ok.hpp"
#include "data/sounds/ui/other.hpp"

#include "builder.hpp"

namespace assets::builder {

assets::textures::TextureManager build_texture_manager()
{
    assets::textures::TextureManager texture_manager;

    // Road textures
    texture_manager.load("top_left", {.data = road_sand89::data, .size = road_sand89::size});
    texture_manager.load("top_right", {.data = road_sand01::data, .size = road_sand01::size});
    texture_manager.load("bottom_right", {.data = road_sand37::data, .size = road_sand37::size});
    texture_manager.load("bottom_left", {.data = road_sand35::data, .size = road_sand35::size});
    texture_manager.load("vertical", {.data = road_sand87::data, .size = road_sand87::size});
    texture_manager.load("horizontal", {.data = road_sand88::data, .size = road_sand88::size});
    texture_manager.load("horizontal_finish", {.data = road_sand39::data, .size = road_sand39::size});

    // Car textures
    texture_manager.load("car_black", {.data = car_black_1::data, .size = car_black_1::size});
    texture_manager.load("car_blue", {.data = car_blue_1::data, .size = car_blue_1::size});
    texture_manager.load("car_green", {.data = car_green_1::data, .size = car_green_1::size});
    texture_manager.load("car_red", {.data = car_red_1::data, .size = car_red_1::size});
    texture_manager.load("car_yellow", {.data = car_yellow_1::data, .size = car_yellow_1::size});

    return texture_manager;
}

assets::sounds::SoundManager build_sound_manager()
{
    assets::sounds::SoundManager sound_manager;

    // Car sounds
    sound_manager.load("engine", {.data = engine::data, .size = engine::size});
    sound_manager.load("tires", {.data = tires::data, .size = tires::size});
    sound_manager.load("hit", {.data = hit::data, .size = hit::size});

    // UI sounds
    sound_manager.load("ok", {.data = ok::data, .size = ok::size});
    sound_manager.load("other", {.data = other::data, .size = other::size});

    return sound_manager;
}

}  // namespace assets::builder
