/**
 * @file world.cpp
 */

#include <array>    // for std::array
#include <cstddef>  // for std::size_t, std::ptrdiff_t
#include <random>   // for std::mt19937, std::uniform_real_distribution, std::uniform_int_distribution
#include <vector>   // for std::vector

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <spdlog/spdlog.h>

#include "world.hpp"

namespace core::world {

Track::Track(const Textures tiles,
             std::mt19937 &rng,
             const TrackConfig &config)
    : tiles_(tiles),  //  Copy the small struct to prevent segfaults
      rng_(rng),
      config_(Track::validate_config(config))
{
    // Build the track immediately on construction
    this->build();
}

[[nodiscard]] const TrackConfig &Track::get_config() const
{
    return this->config_;
}

void Track::set_config(const TrackConfig &config)
{
    const TrackConfig validated_config = Track::validate_config(config);
    // Only rebuild if the configuration actually changes
    if (!(this->config_ == validated_config)) {
        SPDLOG_DEBUG("Config changed, rebuilding track...");
        this->config_ = validated_config;
        this->build();
    }
    else {
        SPDLOG_DEBUG("Config unchanged, skipping track rebuild!");
    }
}

void Track::reset()
{
    const TrackConfig default_config = Track::validate_config(TrackConfig{});  // Validate default compile-time config anyway
    // Only rebuild if the configuration actually changes
    if (!(this->config_ == default_config)) {
        SPDLOG_DEBUG("Config changed during reset, rebuilding track...");
        this->config_ = default_config;
        this->build();
    }
    else {
        SPDLOG_DEBUG("Config is already default, skipping track rebuild!");
    }
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

const std::vector<TrackWaypoint> &Track::get_waypoints() const
{
    return this->waypoints_;
}

const sf::Vector2f &Track::get_finish_position() const
{
    // SPDLOG_DEBUG("Returning finish point at ('{}', '{}') px!", this->finish_point_.x, this->finish_point_.y);
    return this->finish_point_;
}

void Track::draw(sf::RenderTarget &target) const
{
    for (const sf::Sprite &sprite : this->sprites_) {
        target.draw(sprite);
    }
}

TrackConfig Track::validate_config(const TrackConfig &config)
{
    SPDLOG_DEBUG("Validating track config: horizontal_count='{}', vertical_count='{}', size_px='{}', detour_probability='{}'...",
                 config.horizontal_count,
                 config.vertical_count,
                 config.size_px,
                 config.detour_probability);

    // Create a copy of the config, will be modified
    TrackConfig validated_config = config;

    // Define minimum and maximum allowable values
    constexpr std::size_t min_tile_count = 3;       // Minimum number of tiles in each direction
    constexpr std::size_t min_size_px = 256;        // Minimum texture size; cars are 71x131 pixels so this seems reasonable
    constexpr float min_detour_probability = 0.0f;  // Probability must be in [0.0, 1.0]
    constexpr float max_detour_probability = 1.0f;

    // Check and clamp values; all are marked as [[unlikely]] because they should not happen in normal operation, this is defensive programming
    if (validated_config.horizontal_count < min_tile_count) [[unlikely]] {
        SPDLOG_WARN("horizontal_count '{}' is too small; using '{}'", validated_config.horizontal_count, min_tile_count);
        validated_config.horizontal_count = min_tile_count;
    }
    if (validated_config.vertical_count < min_tile_count) [[unlikely]] {
        SPDLOG_WARN("vertical_count '{}' is too small; using '{}'", validated_config.vertical_count, min_tile_count);
        validated_config.vertical_count = min_tile_count;
    }
    if (validated_config.size_px < min_size_px) [[unlikely]] {
        SPDLOG_WARN("size_px '{}' is too small; using '{}'", validated_config.size_px, min_size_px);
        validated_config.size_px = min_size_px;
    }
    if (validated_config.detour_probability < min_detour_probability) [[unlikely]] {
        SPDLOG_WARN("detour_probability '{}' is below '0'; clamping to '0'", validated_config.detour_probability);
        validated_config.detour_probability = min_detour_probability;
    }
    else if (validated_config.detour_probability > max_detour_probability) [[unlikely]] {
        SPDLOG_WARN("detour_probability '{}' exceeds '1'; clamping to '1'", validated_config.detour_probability);
        validated_config.detour_probability = max_detour_probability;
    }

    SPDLOG_DEBUG("Config validated, now returning it!");
    return validated_config;
}

void Track::build()
{
    SPDLOG_DEBUG("Starting build with: horizontal_count='{}', vertical_count='{}', size_px='{}', detour_probability='{}'...",
                 this->config_.horizontal_count,
                 this->config_.vertical_count,
                 this->config_.size_px,
                 this->config_.detour_probability);

    // Reset everything
    this->sprites_.clear();
    this->collision_bounds_.clear();
    this->waypoints_.clear();
    this->finish_point_ = {0.f, 0.f};  // Perhaps not needed, but just in case

    // Reserve capacity
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

    // Temporary vector to collect waypoints in build order, then reorder them starting from finish line
    std::vector<TrackWaypoint> temp_waypoints;
    temp_waypoints.reserve(base_tile_count * 2);
    std::size_t temp_finish_index = 0;

    // Create a sprite from a texture, scale, center, position it, and collect waypoint data
    const auto place_tile = [&](const sf::Texture &texture,
                                const sf::Vector2f &position,
                                const bool is_corner,
                                const bool is_finish = false) {
        // Create a new sprite using the texture
        this->sprites_.emplace_back(texture);
        sf::Sprite &sprite = this->sprites_.back();

        // Set the origin to the center of the sprite, for easier positioning
        sprite.setOrigin(sprite.getLocalBounds().getCenter());

        // Scale the sprite to the desired size
        sprite.setScale({scale_factor, scale_factor});

        // Set the position of the sprite to the provided position
        sprite.setPosition(position);

        // If it's the finish line, record its position to be used as a spawn point
        // This should be true only once throughout the entire track; you cannot have multiple finish points
        if (is_finish) {
            this->finish_point_ = position;
            // Record the finish index in the temporary build order
            temp_finish_index = temp_waypoints.size();
        }

        // Add waypoint to temporary collection in build order
        temp_waypoints.emplace_back(TrackWaypoint{position,
                                                  is_corner
                                                      ? TrackWaypoint::DrivingType::Corner
                                                      : TrackWaypoint::DrivingType::Straight});
    };

    // Define bubble sizes allowed for detours
    constexpr std::array<std::size_t, 2> bubble_heights = {3, 4};

    // Define distribution for detour chance [0.0, 1.0]
    std::uniform_real_distribution<float> detour_dist{0.0f, 1.0f};

    // Process the edge, walking downward and laying optional detours
    const auto process_edge_down = [&](const float main_x,
                                       const float detour_x,
                                       const sf::Texture &top_detour,
                                       const sf::Texture &top_main,
                                       const sf::Texture &bottom_detour,
                                       const sf::Texture &bottom_main) {
        for (std::size_t row = 1; row < this->config_.vertical_count - 1;) {
            if (detour_dist(this->rng_) < this->config_.detour_probability) {
                // Determine which bubble heights fit
                std::vector<std::size_t> viable;
                for (const std::size_t height : bubble_heights) {
                    if (row + height < this->config_.vertical_count) {
                        viable.emplace_back(height);
                    }
                }
                if (!viable.empty()) {
                    // Choose a random bubble height
                    // If only one viable height, use it; otherwise pick a random one
                    const std::size_t height = viable.size() == 1
                                                   ? viable.front()
                                                   : viable[std::uniform_int_distribution<std::size_t>(0, viable.size() - 1)(this->rng_)];
                    const float y_top = top_left_origin.y + (static_cast<float>(row) + half) * tile_size;
                    const float y_bottom = y_top + static_cast<float>(height - 1) * tile_size;

                    // Place entry curves
                    place_tile(top_main,
                               {main_x, y_top},
                               true);
                    place_tile(top_detour,
                               {detour_x, y_top},
                               true);

                    // Place straight detour verticals
                    for (std::size_t offset = 1; offset + 1 < height; ++offset) {
                        const float y_mid = y_top + static_cast<float>(offset) * tile_size;
                        place_tile(this->tiles_.vertical,
                                   {detour_x, y_mid},
                                   false);
                    }

                    // Place the bottom tiles of the detour segment
                    place_tile(bottom_detour,
                               {detour_x, y_bottom},
                               true);
                    place_tile(bottom_main,
                               {main_x, y_bottom},
                               true);

                    // Advance row pointer beyond detour and insert continuity tile
                    // This is a fix for the real edge not having a vertical tile before the next detour
                    row += height;
                    if (row < this->config_.vertical_count - 1) {
                        const float y_cont = top_left_origin.y + (static_cast<float>(row) + half) * tile_size;
                        place_tile(this->tiles_.vertical,
                                   {main_x, y_cont},
                                   false);
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
            ++row;
        }
    };

    // Process the edge, walking upward and laying optional detours
    const auto process_edge_up = [&](const float main_x,
                                     const float detour_x,
                                     const sf::Texture &bottom_detour,
                                     const sf::Texture &bottom_main,
                                     const sf::Texture &top_detour,
                                     const sf::Texture &top_main) {
        for (std::ptrdiff_t row = static_cast<std::ptrdiff_t>(this->config_.vertical_count - 2); row > 0;) {
            if (detour_dist(this->rng_) < this->config_.detour_probability) {
                // Determine which bubble heights fit
                std::vector<std::size_t> viable;
                for (const std::size_t height : bubble_heights) {
                    const std::ptrdiff_t height_signed = static_cast<std::ptrdiff_t>(height);
                    if (row >= height_signed) {
                        viable.emplace_back(height);
                    }
                }
                if (!viable.empty()) {
                    // Choose a random bubble height
                    // If only one viable height, use it; otherwise pick a random one
                    const std::size_t height = viable.size() == 1
                                                   ? viable.front()
                                                   : viable[std::uniform_int_distribution<std::size_t>(0, viable.size() - 1)(this->rng_)];
                    const float y_bottom = top_left_origin.y + (static_cast<float>(row) + half) * tile_size;
                    const float y_top = y_bottom - static_cast<float>(height - 1) * tile_size;

                    // Place entry curves
                    place_tile(bottom_main,
                               {main_x, y_bottom},
                               true);
                    place_tile(bottom_detour,
                               {detour_x, y_bottom},
                               true);

                    // Place straight detour verticals
                    for (std::size_t off = 1; off + 1 < height; ++off) {
                        const float y_mid = y_bottom - static_cast<float>(off) * tile_size;
                        place_tile(this->tiles_.vertical,
                                   {detour_x, y_mid},
                                   false);
                    }

                    // Place the bottom tiles of the detour segment
                    place_tile(top_detour,
                               {detour_x, y_top},
                               true);
                    place_tile(top_main,
                               {main_x, y_top},
                               true);

                    // Advance row pointer beyond detour and insert continuity tile
                    // This is a fix for the real edge not having a vertical tile before the next detour
                    row -= static_cast<std::ptrdiff_t>(height);
                    if (row > 0) {
                        const float y_cont = top_left_origin.y + (static_cast<float>(row) + half) * tile_size;
                        place_tile(this->tiles_.vertical,
                                   {main_x, y_cont},
                                   false);
                        --row;
                    }
                    continue;
                }
            }

            // No detour: plain vertical
            const float y = top_left_origin.y + (static_cast<float>(row) + half) * tile_size;
            place_tile(this->tiles_.vertical,
                       {main_x, y},
                       false);
            --row;
        }
    };

    // Pre compute positions
    const float top_row_y = top_left_origin.y + half * tile_size;
    const float bottom_row_y = top_left_origin.y + total_height - half * tile_size;
    const float left_main_x = top_left_origin.x + half * tile_size;
    const float right_main_x = top_left_origin.x + total_width - half * tile_size;
    const float left_detour_x = left_main_x - tile_size;
    const float right_detour_x = right_main_x + tile_size;

    // Place top left corner
    place_tile(this->tiles_.top_left,
               {left_main_x, top_row_y},
               true);

    // Place top edge left to right
    const std::size_t finish_idx = 1 + (this->config_.horizontal_count - 2) / 2;
    if (this->config_.horizontal_count % 2 == 0) {
        SPDLOG_WARN("Horizontal tile count '{}' is even, the finish line will be placed at an uneven, right-of-center, index '{}'!", this->config_.horizontal_count, finish_idx);
    }
    for (std::size_t x_index = 1; x_index < this->config_.horizontal_count - 1; ++x_index) {
        const float x = top_left_origin.x + (static_cast<float>(x_index) + half) * tile_size;
        const bool is_finish = x_index == finish_idx;
        place_tile(is_finish
                       ? this->tiles_.horizontal_finish
                       : this->tiles_.horizontal,
                   {x, top_row_y},
                   false,
                   is_finish);
    }

    // Place top right corner
    place_tile(this->tiles_.top_right,
               {right_main_x, top_row_y},
               true);

    // Place right edge downward
    process_edge_down(right_main_x,
                      right_detour_x,
                      this->tiles_.top_right,
                      this->tiles_.bottom_left,
                      this->tiles_.bottom_right,
                      this->tiles_.top_left);

    // Place bottom right corner
    place_tile(this->tiles_.bottom_right,
               {right_main_x, bottom_row_y},
               true);

    // Place bottom edge right to left
    for (std::size_t x_index = this->config_.horizontal_count - 2; x_index > 0; --x_index) {
        const float x = top_left_origin.x + (static_cast<float>(x_index) + half) * tile_size;
        place_tile(this->tiles_.horizontal,
                   {x, bottom_row_y},
                   false);
    }

    // Place bottom left corner
    place_tile(this->tiles_.bottom_left,
               {left_main_x, bottom_row_y},
               true);

    // Place left edge upward
    process_edge_up(left_main_x,
                    left_detour_x,
                    this->tiles_.bottom_left,
                    this->tiles_.top_right,
                    this->tiles_.top_left,
                    this->tiles_.bottom_right);

    // Pre-cache collision bounds for all sprites
    for (const auto &sprite : this->sprites_) {
        this->collision_bounds_.emplace_back(sprite.getGlobalBounds());
    }

    // Reorder waypoints to start from the finish line position
    // This eliminates the need for finish_index_ workaround in AI navigation
    SPDLOG_DEBUG("Now reordering waypoints: finish line is at index '{}' (out of '{}' total)", temp_finish_index, temp_waypoints.size());

    this->waypoints_.clear();
    this->waypoints_.reserve(temp_waypoints.size());

    // Add waypoints starting from finish line, going forward in racing direction
    for (std::size_t offset = 0; offset < temp_waypoints.size(); ++offset) {
        const std::size_t index = (temp_finish_index + offset) % temp_waypoints.size();
        this->waypoints_.emplace_back(temp_waypoints[index]);
    }

    SPDLOG_DEBUG("Waypoints reordered, now starting from the finish line at index '0' (out of '{}' total)", this->waypoints_.size());

    // Shrink containers
    this->sprites_.shrink_to_fit();
    this->waypoints_.shrink_to_fit();
    this->collision_bounds_.shrink_to_fit();
    SPDLOG_DEBUG("Track consisting of '{}' tiles built successfully!", this->sprites_.size());
}

}  // namespace core::world
