/**
 * @file game.cpp
 */

#include <array>    // for std::array
#include <cstddef>  // for std::size_t
#include <random>   // for std::mt19937, std::uniform_real_distribution, std::uniform_int_distribution
#include <vector>   // for std::vector

#include <SFML/Graphics.hpp>
#include <spdlog/spdlog.h>

#include "game.hpp"

namespace core::game {

Track::Track(const TrackTiles &tiles,
             std::mt19937 &rng,
             const TrackConfig &config)
    : tiles_(tiles),
      rng_(rng),
      config_(config)
{
    SPDLOG_DEBUG("Creating Track with config: horizontal_count='{}', vertical_count='{}', size_px='{}', detour_chance_pct='{}'...",
                 this->config_.horizontal_count,
                 this->config_.vertical_count,
                 this->config_.size_px,
                 this->config_.detour_chance_pct);

    // Build the track on construction
    this->build();

    SPDLOG_DEBUG("Track created successfully, exiting constructor!");
}

void Track::set_config(const TrackConfig &config)
{
    // SPDLOG_DEBUG("Setting new config: horizontal_count='{}', vertical_count='{}', size_px='{}', detour_chance_pct='{}'...",
    //              config.horizontal_count,
    //              config.vertical_count,
    //              config.size_px,
    //              config.detour_chance_pct);
    // Rebuild only when necessary
    // if (this->config_ != config) [[likely]] {  // Probably likely if the user is calling this method
    //     SPDLOG_DEBUG("Config changed, rebuilding track...");
    this->config_ = config;
    this->build();
    //     SPDLOG_DEBUG("Track rebuilt successfully!");
    // }
    // else {
    //     SPDLOG_DEBUG("Config is the same, skipping rebuild...");
    // }
}

[[nodiscard]] const TrackConfig &Track::get_config() const
{
    return this->config_;
}

const sf::Vector2f &Track::get_finish_point() const
{
    // SPDLOG_DEBUG("Returning finish point at ('{}', '{}') px!", this->finish_point_.x, this->finish_point_.y);
    return this->finish_point_;
}

bool Track::is_on_track(const sf::Vector2f &world_position) const
{
    for (const auto &bounds : this->collision_bounds_) {
        if (bounds.contains(world_position)) {
            return true;
        }
    }
    return false;
}

std::vector<Waypoint> Track::get_waypoints() const
{
    return this->waypoints_;
}

void Track::draw(sf::RenderTarget &target) const
{
    for (const sf::Sprite &sprite : this->sprites_) {
        target.draw(sprite);
    }
}

void Track::build()
{
    SPDLOG_DEBUG("Starting build with: horizontal_count='{}', vertical_count='{}', size_px='{}', detour_chance_pct='{}'...",
                 this->config_.horizontal_count,
                 this->config_.vertical_count,
                 this->config_.size_px,
                 this->config_.detour_chance_pct);

    // Reset sprites and reserve capacity
    this->sprites_.clear();
    this->waypoints_.clear();
    this->collision_bounds_.clear();
    const std::size_t base_tile_count =
        4                                           // Corners
        + 2 * (this->config_.horizontal_count - 2)  // Top and bottom edges
        + 2 * (this->config_.vertical_count - 2);   // Left and right edges
    this->sprites_.reserve(base_tile_count * 2);    // Multiply by 2 for detours
    this->waypoints_.reserve(base_tile_count * 2);  // Multiply by 2 for detours
    // SPDLOG_DEBUG("Reserved capacity for '{}' sprites, now calculating track size...", this->sprites_.capacity());

    // Define the half the tile size for centering and positioning
    constexpr float half = 0.5f;

    // Calculate total width and height based on the number of tiles and the desired tile size
    const float tile_size = static_cast<float>(this->config_.size_px);
    const float total_width = tile_size * static_cast<float>(this->config_.horizontal_count);
    const float total_height = tile_size * static_cast<float>(this->config_.vertical_count);

    // Top-left corner position, will be used as a starting point for placing tiles, assuming the track is centered at the origin
    const sf::Vector2f top_left_origin = {-half * total_width,
                                          -half * total_height};

    // Assuming that all map textures are the same size and square (same height & width), get the width of the first one
    // Then, calculate the scale factor based on the desired tile size and the texture size
    // Unfortunately, "sprite.setScale()" expects a float, so we need to convert the final size to float
    const float scale_factor = tile_size / static_cast<float>(this->tiles_.top_left.getSize().x);
    // SPDLOG_DEBUG("Tile size '{}' px gives total size ('{}', '{}') px, top-left origin ('{}', '{}') px, and scale factor '{}x'!",
    //              tile_size,
    //              total_width,
    //              total_height,
    //              top_left_origin.x,
    //              top_left_origin.y,
    //              scale_factor);

    // Create a sprite from a texture, scale, center, position it, and optionally record its position as the finish line
    const auto place_tile = [&](const sf::Texture &texture,
                                const sf::Vector2f &position,
                                const bool is_corner,
                                const bool finish_point = false) {
        // Create a new sprite using the texture
        sf::Sprite &sprite = this->sprites_.emplace_back(texture);  // Emplace, then reference

        // Scale the sprite to the desired size
        sprite.setScale({scale_factor, scale_factor});

        // Set the origin to the center of the sprite, for easier positioning
        sprite.setOrigin(sprite.getLocalBounds().getCenter());

        // Set the position of the sprite to the provided position
        sprite.setPosition(position);

        // If it's a corner tile, add it to the waypoints as a corner, otherwise as a straight line
        if (is_corner) {
            this->waypoints_.emplace_back(Waypoint{position, WaypointType::Corner});
            // SPDLOG_DEBUG("Placed corner waypoint at ('{}', '{}') px!", position.x, position.y);
        }
        else {
            this->waypoints_.emplace_back(Waypoint{position, WaypointType::Straight});
            // SPDLOG_DEBUG("Placed straight waypoint at ('{}', '{}') px!", position.x, position.y);
        }

        // If it's the finish line, record its position to be used as a spawn point
        // This should be true only once throughout the entire track; you cannot have multiple finish points
        if (finish_point) {
            this->finish_point_ = position;
        }
    };

    // Place the four corner tiles
    // SPDLOG_DEBUG("Placing corner tiles...");
    place_tile(this->tiles_.top_left,
               {top_left_origin.x + half * tile_size, top_left_origin.y + half * tile_size},
               true);
    place_tile(this->tiles_.top_right,
               {top_left_origin.x + total_width - half * tile_size, top_left_origin.y + half * tile_size},
               true);
    place_tile(this->tiles_.bottom_right,
               {top_left_origin.x + total_width - half * tile_size, top_left_origin.y + total_height - half * tile_size},
               true);
    place_tile(this->tiles_.bottom_left,
               {top_left_origin.x + half * tile_size, top_left_origin.y + total_height - half * tile_size},
               true);
    // SPDLOG_DEBUG("Corner tiles placed!");

    // Determine which top edge tile becomes the finish line (middle tile excluding corners)
    const std::size_t finish_tile_idx = 1 + (this->config_.horizontal_count - 2) / 2;
    // SPDLOG_DEBUG("Finish line tile index will be placed at: index '{}' (out of '{}')!", finish_tile_idx, this->config_.horizontal_count - 2);
    if (this->config_.horizontal_count % 2 == 0) {
        SPDLOG_WARN("Horizontal tile count '{}' is even, the finish line will be placed at an uneven, right-of-center, index '{}'!", this->config_.horizontal_count, finish_tile_idx);
    }

    // Calculate the positions of the top and bottom edges
    const float top_row_y = top_left_origin.y + half * tile_size;
    const float bottom_row_y = top_left_origin.y + total_height - half * tile_size;

    // Place the top and bottom edge (straight horizontal line)
    // SPDLOG_DEBUG("Placing top and bottom edge tiles at: top_row_y='{}' and bottom_row_y='{}'...", top_row_y, bottom_row_y);
    for (std::size_t x_index = 1; x_index < this->config_.horizontal_count - 1; ++x_index) {
        const float x = top_left_origin.x + (static_cast<float>(x_index) + half) * tile_size;

        // Place the top edge
        // If at the middle point, replace the central tile with the finish line texture and set the finish point variable
        const bool is_finish_line = x_index == finish_tile_idx;
        place_tile(is_finish_line
                       ? this->tiles_.horizontal_finish
                       : this->tiles_.horizontal,
                   {x, top_row_y},
                   false,
                   is_finish_line);

        // Place the bottom edge
        place_tile(this->tiles_.horizontal,
                   {x, bottom_row_y},
                   false);
    }
    // SPDLOG_DEBUG("Top and bottom edge tiles placed, now setting up vertical edges...");

    // Bubble sizes allowed for detours
    constexpr std::array<std::size_t, 2> bubble_heights = {3, 4};

    // Distribution for detour chance [0.0, 1.0]
    std::uniform_real_distribution<float> detour_chance_distribution{0.0f, 1.0f};

    // Process one vertical edge (main_x is real edge, detour_x is extra column), adding detours as needed
    const auto process_edge = [&](float main_x,
                                  float detour_x,
                                  const sf::Texture &top_detour,
                                  const sf::Texture &top_main,
                                  const sf::Texture &bottom_detour,
                                  const sf::Texture &bottom_main) {
        // SPDLOG_DEBUG("Processing vertical edge: main_x='{}' detour_x='{}'...", main_x, detour_x);

        // Iterate over the vertical edge, excluding the top and bottom corner tiles
        for (std::size_t row = 1; row < this->config_.vertical_count - 1;) {
            const float roll = detour_chance_distribution(this->rng_);
            const bool want_detour = roll < this->config_.detour_chance_pct;
            // SPDLOG_DEBUG("At row '{}': roll='{:.3f}', want_detour='{}'", row, roll, want_detour);

            if (want_detour) {
                // Determine which bubble heights fit
                std::vector<std::size_t> viable;
                for (std::size_t height : bubble_heights) {
                    if (row + height < this->config_.vertical_count) {
                        viable.emplace_back(height);
                    }
                }
                // SPDLOG_DEBUG("Viable detour heights: '{}'", viable.size());

                if (!viable.empty()) {
                    // Choose a random bubble height
                    // If only one viable height, use it; otherwise pick a random one
                    const std::size_t height = viable.size() == 1
                                                   ? viable.front()
                                                   : viable[std::uniform_int_distribution<std::size_t>(0, viable.size() - 1)(this->rng_)];
                    // SPDLOG_DEBUG("Chosen detour height: '{}'", height);

                    const float top_row_y_detour = top_left_origin.y + (static_cast<float>(row) + half) * tile_size;
                    const float bottom_row_y_detour = top_row_y_detour + static_cast<float>(height - 1) * tile_size;

                    // Place entry curves
                    place_tile(top_detour,
                               {detour_x, top_row_y_detour},
                               true);
                    place_tile(top_main,
                               {main_x, top_row_y_detour},
                               true);

                    // Place straight detour verticals
                    for (std::size_t offset = 1; offset + 1 < height; ++offset) {
                        const float mid_y = top_row_y_detour + static_cast<float>(offset) * tile_size;
                        place_tile(this->tiles_.vertical,
                                   {detour_x, mid_y},
                                   false);
                        // SPDLOG_DEBUG("Placed detour vertical tile at ('{}', '{}')", detour_x, mid_y);
                    }

                    // Place the bottom tiles of the detour segment
                    place_tile(bottom_detour,
                               {detour_x, bottom_row_y_detour},
                               true);
                    place_tile(bottom_main,
                               {main_x, bottom_row_y_detour},
                               true);

                    // Advance row pointer beyond detour and insert continuity tile
                    // This is a fix for the real edge not having a vertical tile before the next detour
                    row += height;
                    if (row < this->config_.vertical_count - 1) {
                        const float cont_y = top_left_origin.y + (static_cast<float>(row) + half) * tile_size;
                        place_tile(this->tiles_.vertical,
                                   {main_x, cont_y},
                                   false);
                        // SPDLOG_DEBUG("Placed continuity vertical tile at ('{}', '{}')", main_x, cont_y);
                        ++row;
                    }
                    continue;
                }
            }

            // No detour: plain vertical
            const float y = top_left_origin.y + (static_cast<float>(row) + half) * tile_size;
            place_tile(this->tiles_.vertical,
                       {main_x, y},
                       false);
            // SPDLOG_DEBUG("Placed regular vertical tile at ('{}', '{}')", main_x, y);
            ++row;
        }
    };

    // Process the right edge (detours to the right)
    // SPDLOG_DEBUG("Processing right vertical edge...");
    const float right_main_x = top_left_origin.x + total_width - half * tile_size;
    const float right_detour_x = right_main_x + tile_size;
    process_edge(right_main_x,
                 right_detour_x,
                 this->tiles_.top_right,
                 this->tiles_.bottom_left,
                 this->tiles_.bottom_right,
                 this->tiles_.top_left);

    // Process the left edge (detours to the left)
    // SPDLOG_DEBUG("Processing left vertical edge...");
    const float left_main_x = top_left_origin.x + half * tile_size;
    const float left_detour_x = left_main_x - tile_size;
    process_edge(left_main_x,
                 left_detour_x,
                 this->tiles_.top_left,
                 this->tiles_.bottom_right,
                 this->tiles_.bottom_left,
                 this->tiles_.top_right);

    // Pre-cache collision bounds for all sprites
    // SPDLOG_DEBUG("Pre-caching collision bounds for all sprites...");
    for (const auto &sprite : this->sprites_) {
        this->collision_bounds_.emplace_back(sprite.getGlobalBounds());
    }
    // SPDLOG_DEBUG("Collision bounds cached!");

    // Fit storage to the actual sprite count
    this->sprites_.shrink_to_fit();
    this->waypoints_.shrink_to_fit();
    this->collision_bounds_.shrink_to_fit();
    SPDLOG_DEBUG("Track consisting of '{}' tiles built successfully!", this->sprites_.size());
}

}  // namespace core::game
