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
    texture_manager.load("top_left", {road_sand89::data, road_sand89::size});
    texture_manager.load("top_right", {road_sand01::data, road_sand01::size});
    texture_manager.load("bottom_right", {road_sand37::data, road_sand37::size});
    texture_manager.load("bottom_left", {road_sand35::data, road_sand35::size});
    texture_manager.load("vertical", {road_sand87::data, road_sand87::size});
    texture_manager.load("horizontal", {road_sand88::data, road_sand88::size});
    texture_manager.load("horizontal_finish", {road_sand39::data, road_sand39::size});

    // Car textures
    texture_manager.load("car_black", {car_black_1::data, car_black_1::size});
    texture_manager.load("car_blue", {car_blue_1::data, car_blue_1::size});
    texture_manager.load("car_green", {car_green_1::data, car_green_1::size});
    texture_manager.load("car_red", {car_red_1::data, car_red_1::size});
    texture_manager.load("car_yellow", {car_yellow_1::data, car_yellow_1::size});

    return texture_manager;
}

assets::sounds::SoundManager build_sound_manager()
{
    assets::sounds::SoundManager sound_manager;

    // Car sounds
    sound_manager.load("engine", {engine::data, engine::size});
    sound_manager.load("tires", {tires::data, tires::size});
    sound_manager.load("hit", {hit::data, hit::size});

    // UI sounds
    sound_manager.load("ok", {ok::data, ok::size});
    sound_manager.load("other", {other::data, other::size});

    return sound_manager;
}

}  // namespace assets::builder
