/**
 * @file game.cpp
 */

#include <algorithm>  // for std::clamp, std::min, std::max
#include <array>      // for std::array
#include <cmath>      // for std::hypot, std::remainder, std::atan2, std::sin, std::cos, std::lerp, std::abs
#include <cstddef>    // for std::size_t
#include <numbers>    // for std::numbers
#include <random>     // for std::mt19937, std::uniform_real_distribution, std::uniform_int_distribution
#include <vector>     // for std::vector

#include <SFML/Graphics.hpp>
#include <spdlog/spdlog.h>

#include "game.hpp"

namespace core::game {

Track::Track(const Textures tiles,
             std::mt19937 &rng,
             const TrackConfig &config)
    : tiles_(tiles),  //  Copy the small struct to prevent segfaults
      rng_(rng),
      config_(Track::validate_config(config)),
      finish_point_(0.f, 0.f)
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
                for (std::size_t height : bubble_heights) {
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
                for (std::size_t height : bubble_heights) {
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

Car::Car(const sf::Texture &texture,
         std::mt19937 &rng,
         const Track &track,
         const CarControlMode control_mode,
         const CarConfig &config)
    : sprite_(texture),
      track_(track),
      config_(config),
      rng_(rng),
      control_mode_(control_mode),
      last_position_({0.0f, 0.0f}),
      velocity_({0.0f, 0.0f}),
      current_input_(),
      steering_wheel_angle_(0.0f),
      current_waypoint_index_number_(1),
      drift_score_(0.0f)
{
    this->sprite_.setOrigin({this->sprite_.getTexture().getSize().x / 2.0f, this->sprite_.getTexture().getSize().y / 2.0f});
    this->reset();
}

void Car::reset()
{
    // Get spawn position from track
    const sf::Vector2f spawn_position = this->track_.get_finish_position();

    // Place sprite at spawn position
    this->sprite_.setPosition(spawn_position);

    // Point car downward initially (towards first waypoint if available)
    const auto waypoints = this->track_.get_waypoints();
    if (!waypoints.empty() && waypoints.size() > 1) {
        const sf::Vector2f direction_to_first_waypoint = waypoints[1].position - spawn_position;
        const float initial_heading_radians = std::atan2(direction_to_first_waypoint.y, direction_to_first_waypoint.x);
        this->sprite_.setRotation(sf::radians(initial_heading_radians));
    }
    else {
        // Default downward orientation if no waypoints available
        this->sprite_.setRotation(sf::degrees(90.0f));
    }

    // Clear all physics state
    this->velocity_ = {0.0f, 0.0f};
    this->steering_wheel_angle_ = 0.0f;
    this->current_input_ = {};  // Reset input values
    this->last_position_ = spawn_position;

    // Reset AI state
    this->current_waypoint_index_number_ = 1;

    // Reset drift score
    this->drift_score_ = 0.0f;
}

[[nodiscard]] float Car::get_speed() const
{
    return std::hypot(this->velocity_.x, this->velocity_.y);
}

[[nodiscard]] sf::Vector2f Car::get_velocity() const
{
    return this->velocity_;
}

[[nodiscard]] sf::Vector2f Car::get_position() const
{
    return this->sprite_.getPosition();
}

[[nodiscard]] CarControlMode Car::get_control_mode() const
{
    return this->control_mode_;
}

[[nodiscard]] std::size_t Car::get_current_waypoint_index() const
{
    return this->current_waypoint_index_number_;
}

[[nodiscard]] float Car::get_drift_score() const
{
    return this->drift_score_;
}

void Car::set_control_mode(const CarControlMode control_mode)
{
    this->control_mode_ = control_mode;
}

void Car::apply_input(const CarInput &input)
{
    // Store input values directly for analog control
    this->current_input_ = input;
}

void Car::update(const float dt)
{
    // Handle AI behavior if in AI mode
    if (this->control_mode_ == CarControlMode::AI) {
        this->update_ai_behavior(dt);
    }

    // Apply physics regardless of control mode
    this->apply_physics_step(dt);
}

void Car::draw(sf::RenderTarget &target) const
{
    target.draw(this->sprite_);
}

void Car::update_ai_behavior([[maybe_unused]] const float dt)
{
    // AI behavior constants
    static constexpr float waypoint_reach_factor = 0.65f;                        // Distance factor for waypoint reach detection; increase = reach waypoints from farther away, decrease = must get closer to reach waypoints
    static constexpr float collision_distance = 0.65f;                           // Distance ahead to check for collisions as fraction of tile size; increase = avoid collisions earlier, decrease = check collisions closer to car
    static constexpr float straight_steering_threshold = 0.25f;                  // Heading difference threshold for steering on straights in radians; increase = less sensitive steering on straights, decrease = more twitchy steering on straights
    static constexpr float corner_steering_threshold = 0.08f;                    // Heading difference threshold for steering in corners in radians; increase = less responsive cornering, decrease = more aggressive cornering
    static constexpr float minimum_straight_steering_difference = 0.1f;          // Minimum heading difference for straight steering in radians; increase = reduce steering wobble but less precision, decrease = more precise but potentially jittery steering
    static constexpr float early_corner_turn_distance = 1.0f;                    // Distance factor for early corner turning; increase = start turning earlier before corners, decrease = turn later and sharper into corners
    static constexpr float corner_speed_factor = 1.2f;                           // Speed multiplier for corners as fraction of tile size; increase = faster through corners but riskier, decrease = slower and safer through corners
    static constexpr float straight_speed_factor = 3.0f;                         // Speed multiplier for straights as fraction of tile size; increase = faster on straights, decrease = slower and more conservative on straights
    static constexpr float brake_distance_factor = 3.0f;                         // Distance factor for braking before corners; increase = start braking earlier before corners, decrease = brake later and more aggressively
    static constexpr float random_variation_minimum = 0.8f;                      // Minimum random variation multiplier; increase = less variation and more predictable AI, decrease = more erratic AI behavior
    static constexpr float random_variation_maximum = 1.2f;                      // Maximum random variation multiplier; increase = more chaotic AI behavior, decrease = more consistent and predictable AI
    static constexpr float collision_velocity_threshold_factor = 0.1f;           // Minimum speed for collision checking as fraction of tile size; increase = only check collisions at higher speeds, decrease = check collisions even at very low speeds
    static constexpr float max_heading_for_full_steering_degrees = 45.0f;        // Degrees for maximum steering intensity; increase = require larger heading errors for full steering, decrease = apply full steering with smaller heading errors
    static constexpr float minimum_steering_intensity = 0.9f;                    // Minimum steering amount to avoid gentle steering; increase = always steer aggressively, decrease = allow more gentle steering corrections
    static constexpr float corner_minimum_steering_difference_radians = 0.001f;  // Minimum heading difference for corner steering in radians; increase = reduce corner steering sensitivity, decrease = increase corner steering sensitivity
    static constexpr float overspeed_braking_threshold_factor = 1.5f;            // Speed multiplier for emergency braking; increase = allow higher speeds before emergency braking, decrease = trigger emergency braking at lower speeds
    static constexpr float significant_speed_reduction_threshold_factor = 0.4f;  // Speed difference threshold for significant braking as fraction of tile size; increase = require larger speed differences to brake hard, decrease = brake hard with smaller speed differences
    static constexpr float speed_increase_threshold_factor = 0.2f;               // Speed difference threshold for acceleration as fraction of tile size; increase = require larger speed deficits to accelerate, decrease = accelerate with smaller speed deficits
    static constexpr float braking_speed_overage_factor = 0.5f;                  // Fraction over target speed that triggers full brake; increase = tolerate higher speeds before full braking, decrease = apply full brake closer to target speed
    static constexpr float throttle_speed_underage_factor = 0.8f;                // Fraction under target speed that triggers full throttle; increase = tolerate lower speeds before full throttle, decrease = apply full throttle closer to target speed
    static constexpr float gentle_speed_difference_threshold_factor = 0.5f;      // Factor for gentle speed adjustments relative to speed_increase_threshold; increase = use gentle adjustments less often, decrease = use gentle adjustments more often
    static constexpr float gentle_throttle_maximum = 0.3f;                       // Maximum throttle for gentle speed adjustments; increase = more aggressive gentle acceleration, decrease = more conservative gentle acceleration
    static constexpr float gentle_brake_maximum = 0.3f;                          // Maximum brake for gentle speed adjustments; increase = more aggressive gentle braking, decrease = more conservative gentle braking

    // Reset input values at start of AI update
    this->current_input_ = {};

    // Get basic info
    const auto &waypoints = this->track_.get_waypoints();

    // Safety check for empty waypoints
    if (waypoints.empty()) {
        SPDLOG_WARN("No waypoints available, cannot update AI car!");
        return;
    }

    const std::size_t current_index = this->current_waypoint_index_number_;
    const std::size_t next_index = (current_index + 1) % waypoints.size();
    const TrackWaypoint &current_waypoint = waypoints[current_index];
    const TrackWaypoint &next_waypoint = waypoints[next_index];
    const sf::Vector2f car_position = this->sprite_.getPosition();
    const float tile_size = static_cast<float>(this->track_.get_config().size_px);
    const float current_speed = this->get_speed();

    // Cache random variations to avoid multiple RNG calls per parameter
    std::uniform_real_distribution<float> random_distribution(random_variation_minimum, random_variation_maximum);
    const float speed_random_variation = random_distribution(this->rng_);
    const float steering_random_variation = random_distribution(this->rng_);
    const float distance_random_variation = random_distribution(this->rng_);

    // Calculate distances
    const sf::Vector2f vector_to_current_waypoint = current_waypoint.position - car_position;
    const float distance_to_current_waypoint = std::hypot(vector_to_current_waypoint.x, vector_to_current_waypoint.y);
    const float waypoint_reach_distance = tile_size * waypoint_reach_factor;

    // Collision detection - check one point ahead based on velocity direction
    bool collision_detected = false;
    const sf::Vector2f car_velocity = this->get_velocity();
    const float velocity_magnitude = std::hypot(car_velocity.x, car_velocity.y);
    const float collision_velocity_threshold = tile_size * collision_velocity_threshold_factor;
    if (velocity_magnitude > collision_velocity_threshold) {
        const sf::Vector2f velocity_normalized = car_velocity / velocity_magnitude;
        const sf::Vector2f check_point = car_position + velocity_normalized * (tile_size * collision_distance);
        collision_detected = !this->track_.is_on_track(check_point);
    }

    // Determine driving context for better decision making
    const bool at_corner = (current_waypoint.type == TrackWaypoint::DrivingType::Corner);
    const bool approaching_corner = (next_waypoint.type == TrackWaypoint::DrivingType::Corner);
    const bool in_corner_context = at_corner || approaching_corner;

    // Steering logic with proportional control
    const float desired_heading_radians = std::atan2(vector_to_current_waypoint.y, vector_to_current_waypoint.x);
    const float current_heading_radians = this->sprite_.getRotation().asRadians();
    const float heading_difference_radians = std::remainder(desired_heading_radians - current_heading_radians, 2.0f * std::numbers::pi_v<float>);

    // Dynamic steering threshold based on context and early corner turning
    float steering_threshold = in_corner_context ? corner_steering_threshold : straight_steering_threshold;
    steering_threshold *= steering_random_variation;

    // Early corner turning: if approaching corner and close enough, use corner threshold
    if (approaching_corner && distance_to_current_waypoint < tile_size * early_corner_turn_distance * distance_random_variation) {
        steering_threshold = corner_steering_threshold * steering_random_variation;
    }

    // Apply steering with smoothing to reduce wiggling
    const bool should_steer = collision_detected || std::abs(heading_difference_radians) > steering_threshold;
    const float minimum_steering_difference = in_corner_context ? corner_minimum_steering_difference_radians : minimum_straight_steering_difference;

    if (should_steer && std::abs(heading_difference_radians) > minimum_steering_difference) {
        // Proportional steering based on heading difference for smoother control
        const float max_heading_for_full_steer = sf::degrees(max_heading_for_full_steering_degrees).asRadians();
        float steering_intensity = std::clamp(heading_difference_radians / max_heading_for_full_steer, -1.0f, 1.0f);

        // Apply minimum steering amount to avoid too gentle steering
        if (std::abs(steering_intensity) > 0.0f) {
            steering_intensity = std::copysign(std::max(std::abs(steering_intensity), minimum_steering_intensity), steering_intensity);
        }

        this->current_input_.steering = steering_intensity;
    }

    // Speed management based on driving context
    const float base_target_speed = in_corner_context
                                        ? tile_size * corner_speed_factor
                                        : tile_size * straight_speed_factor;

    const float target_speed = base_target_speed * speed_random_variation;
    const float brake_distance = tile_size * brake_distance_factor * distance_random_variation;

    // Intelligent braking logic
    const bool approaching_corner_too_fast = approaching_corner &&
                                             (distance_to_current_waypoint < brake_distance) &&
                                             (current_speed > target_speed * overspeed_braking_threshold_factor * speed_random_variation);

    const float speed_difference = target_speed - current_speed;
    const float significant_speed_reduction_threshold = tile_size * significant_speed_reduction_threshold_factor;
    const float speed_increase_threshold = tile_size * speed_increase_threshold_factor;

    // Emergency braking for collisions or approaching corners too fast
    const bool emergency_brake = collision_detected || approaching_corner_too_fast;

    // Smooth pedal operation with mutually exclusive throttle/brake logic
    if (emergency_brake) {
        this->current_input_.brake = 1.0f;
        this->current_input_.throttle = 0.0f;  // Ensure throttle is off during emergency braking
    }
    else if (speed_difference < -significant_speed_reduction_threshold) {
        // Proportional braking based on how much we need to slow down
        const float max_speed_over = target_speed * braking_speed_overage_factor;
        const float speed_over = current_speed - target_speed;
        const float brake_intensity = std::clamp(speed_over / max_speed_over, 0.0f, 1.0f);
        this->current_input_.brake = brake_intensity;
        this->current_input_.throttle = 0.0f;  // Ensure throttle is off when braking
    }
    else if (speed_difference > speed_increase_threshold) {
        // Proportional throttle based on how much we need to speed up
        const float max_speed_under = target_speed * throttle_speed_underage_factor;
        const float throttle_intensity = std::clamp(speed_difference / max_speed_under, 0.0f, 1.0f);
        this->current_input_.throttle = throttle_intensity;
        this->current_input_.brake = 0.0f;  // Ensure brake is off when accelerating
    }
    else {
        // Speed is close to target - use gentle control or let engine drag adjust naturally
        const float gentle_speed_difference_threshold = speed_increase_threshold * gentle_speed_difference_threshold_factor;
        if (speed_difference > gentle_speed_difference_threshold) {
            // Apply gentle throttle for fine speed adjustments
            const float gentle_throttle = std::clamp(speed_difference / speed_increase_threshold, 0.0f, gentle_throttle_maximum);
            this->current_input_.throttle = gentle_throttle;
            this->current_input_.brake = 0.0f;
        }
        else if (speed_difference < -gentle_speed_difference_threshold) {
            // Apply gentle braking for fine speed adjustments
            const float gentle_brake = std::clamp(-speed_difference / significant_speed_reduction_threshold, 0.0f, gentle_brake_maximum);
            this->current_input_.brake = gentle_brake;
            this->current_input_.throttle = 0.0f;
        }
        else {
            // Let engine drag naturally adjust speed when very close to target
            this->current_input_.throttle = 0.0f;
            this->current_input_.brake = 0.0f;
        }
    }

    // Advance waypoint with randomized reach distance
    const float randomized_waypoint_reach_distance = waypoint_reach_distance * distance_random_variation;
    if (distance_to_current_waypoint < randomized_waypoint_reach_distance) {
        this->current_waypoint_index_number_ = next_index;
    }
}

void Car::apply_physics_step(const float dt)
{
    // Compute forward unit vector from current heading
    const float heading_angle_radians = this->sprite_.getRotation().asRadians();
    const sf::Vector2f forward_unit_vector = {std::cos(heading_angle_radians), std::sin(heading_angle_radians)};

    // Storage for current speed
    float current_speed = this->get_speed();

    // Apply gas throttle along forward axis (using analog input)
    if (this->current_input_.throttle > 0.0f) {
        const float throttle_force = this->current_input_.throttle * this->config_.throttle_acceleration_rate_pixels_per_second_squared * dt;
        this->velocity_ += forward_unit_vector * throttle_force;
        current_speed = this->get_speed();
    }

    // Apply foot brake deceleration (using analog input)
    if (this->current_input_.brake > 0.0f && current_speed > this->config_.stopped_speed_threshold_pixels_per_second) {
        const float brake_force = this->current_input_.brake * this->config_.brake_deceleration_rate_pixels_per_second_squared * dt;
        const float brake_reduction = std::min(brake_force, current_speed);
        const sf::Vector2f velocity_unit_vector = this->velocity_ / current_speed;
        this->velocity_ -= velocity_unit_vector * brake_reduction;
        current_speed -= brake_reduction;
    }

    // Apply handbrake deceleration (using analog input)
    if (this->current_input_.handbrake > 0.0f && current_speed > this->config_.stopped_speed_threshold_pixels_per_second) {
        const float handbrake_force = this->current_input_.handbrake * this->config_.handbrake_deceleration_rate_pixels_per_second_squared * dt;
        const float new_speed = current_speed - handbrake_force;
        if (new_speed < 0.1f) {
            this->velocity_ = {0.0f, 0.0f};
            current_speed = 0.0f;
        }
        else {
            const sf::Vector2f velocity_unit_vector = this->velocity_ / current_speed;
            this->velocity_ = velocity_unit_vector * new_speed;
            current_speed = new_speed;
        }
    }

    // Apply passive engine drag when no controls are active
    const bool has_throttle_input = this->current_input_.throttle > 0.0f;
    const bool has_brake_input = this->current_input_.brake > 0.0f;
    const bool has_handbrake_input = this->current_input_.handbrake > 0.0f;
    if (!has_throttle_input && !has_brake_input && !has_handbrake_input && current_speed > this->config_.stopped_speed_threshold_pixels_per_second) {
        const float drag = this->config_.engine_braking_rate_pixels_per_second_squared * dt;
        const float speed_after_drag = std::max(current_speed - drag, 0.0f);
        const float drag_scale = (current_speed > 0.0f) ? speed_after_drag / current_speed : 0.0f;
        this->velocity_.x *= drag_scale;
        this->velocity_.y *= drag_scale;
        current_speed = speed_after_drag;
    }

    // Cap speed to configured maximum
    if (current_speed > this->config_.maximum_movement_pixels_per_second) {
        const float max_speed_scale = this->config_.maximum_movement_pixels_per_second / current_speed;
        this->velocity_.x *= max_speed_scale;
        this->velocity_.y *= max_speed_scale;
        current_speed = this->config_.maximum_movement_pixels_per_second;
    }

    // Separate velocity into forward and lateral components
    const float signed_forward_speed = forward_unit_vector.x * this->velocity_.x + forward_unit_vector.y * this->velocity_.y;
    const sf::Vector2f forward_velocity_vector = forward_unit_vector * signed_forward_speed;
    const sf::Vector2f lateral_velocity_vector = this->velocity_ - forward_velocity_vector;

    // Dampen lateral slip for arcade feel
    const float slip_damping_ratio = 1.0f - std::clamp(this->config_.lateral_slip_damping_coefficient_per_second * dt, 0.0f, 1.0f);
    this->velocity_ = forward_velocity_vector + lateral_velocity_vector * slip_damping_ratio;

    // Calculate drift score based on lateral slip velocity and car speed
    const float lateral_speed = std::hypot(lateral_velocity_vector.x, lateral_velocity_vector.y);
    const float drift_threshold_pixels_per_second = 50.0f;              // Minimum lateral speed to count as drifting
    const float speed_multiplier_threshold_pixels_per_second = 100.0f;  // Speed at which drift score multiplier reaches 1.0

    if (lateral_speed > drift_threshold_pixels_per_second && current_speed > drift_threshold_pixels_per_second) {
        // Calculate drift score multiplier based on forward speed (faster = more points)
        const float speed_multiplier = std::min(current_speed / speed_multiplier_threshold_pixels_per_second, 2.0f);

        // Calculate drift angle factor (more sideways = more points)
        const float drift_angle_factor = lateral_speed / (current_speed + 1.0f);  // Avoid division by zero

        // Base drift points per second when drifting
        const float base_drift_points_per_second = 100.0f;

        // Calculate final drift score increment
        const float drift_score_increment = base_drift_points_per_second * speed_multiplier * drift_angle_factor * dt;
        this->drift_score_ += drift_score_increment;

        // Debug logging for drift detection (uncomment for testing)
        // SPDLOG_DEBUG("Drifting! Lateral speed: {:.1f}, Forward speed: {:.1f}, Score increment: {:.2f}, Total score: {:.1f}", lateral_speed, current_speed, drift_score_increment, this->drift_score_);
    }

    // Update steering wheel angle from analog input
    if (std::abs(this->current_input_.steering) > 0.01f) {
        // Apply steering based on analog input value (-1.0 to 1.0)
        const float steering_rate = this->current_input_.steering * this->config_.steering_turn_rate_degrees_per_second * dt;
        this->steering_wheel_angle_ += steering_rate;
    }
    else {
        // Auto center steering wheel when no steering input is active
        if (std::abs(this->steering_wheel_angle_) > this->config_.steering_autocenter_epsilon_degrees && current_speed > 0.0f) {
            const float centering_factor = std::clamp(this->config_.steering_autocenter_rate_degrees_per_second * dt / std::abs(this->steering_wheel_angle_), 0.0f, 1.0f);
            this->steering_wheel_angle_ = std::lerp(this->steering_wheel_angle_, 0.0f, centering_factor);
        }
        else {
            this->steering_wheel_angle_ = 0.0f;
        }
    }

    // Clamp steering wheel angle to physical limits
    this->steering_wheel_angle_ = std::clamp(this->steering_wheel_angle_, -this->config_.maximum_steering_angle_degrees, this->config_.maximum_steering_angle_degrees);

    // Rotate sprite if moving forward or backward faster than threshold
    if (std::abs(signed_forward_speed) > this->config_.minimum_speed_for_rotation_pixels_per_second) {
        const float speed_ratio = std::clamp(current_speed / this->config_.maximum_movement_pixels_per_second, 0.0f, 1.0f);
        const float steering_sensitivity = this->config_.steering_sensitivity_at_zero_speed * (1.0f - speed_ratio) + this->config_.steering_sensitivity_at_maximum_speed * speed_ratio;
        const float direction_sign = (signed_forward_speed >= 0.0f) ? 1.0f : -1.0f;
        // Flip steering when reversing
        this->sprite_.rotate(sf::degrees(direction_sign * this->steering_wheel_angle_ * steering_sensitivity * dt));
    }

    // Move sprite according to velocity
    this->sprite_.move(this->velocity_ * dt);

    // Handle collision with track boundaries
    if (!this->track_.is_on_track(this->sprite_.getPosition())) {
        this->sprite_.setPosition(this->last_position_);
        // Restore last legal position
        this->velocity_ = -this->velocity_ * this->config_.collision_velocity_retention_ratio;
        current_speed = this->get_speed();
        // If below minimum speed, stop the car completely to avoid jitter
        if (current_speed < this->config_.collision_minimum_bounce_speed_pixels_per_second) {
            this->velocity_ = {0.0f, 0.0f};
            // SPDLOG_DEBUG("Collision below minimum bounce speed; now stopping car!");
        }
        // Otherwise, bounce it randomly with speed-scaled angles to avoid jitter at low speeds
        // This is a simple approximation of a bounce, not a real physics simulation; we use a random angle to make the bounce direction unpredictable
        else {
            // Calculate speed ratio from 0.0 (minimum bounce speed) to 1.0 (maximum speed)
            // Protect against division by zero
            float speed_ratio = 0.0f;
            if (this->config_.maximum_movement_pixels_per_second > this->config_.collision_minimum_bounce_speed_pixels_per_second) {
                speed_ratio = std::clamp((current_speed - this->config_.collision_minimum_bounce_speed_pixels_per_second) / (this->config_.maximum_movement_pixels_per_second - this->config_.collision_minimum_bounce_speed_pixels_per_second), 0.0f, 1.0f);
            }

            // Interpolate between minimum and maximum jitter angles based on speed
            const float max_jitter_angle_degrees = this->config_.collision_minimum_random_bounce_angle_degrees * (1.0f - speed_ratio) + this->config_.collision_maximum_random_bounce_angle_degrees * speed_ratio;

            // Generate random angle within the calculated range
            std::uniform_real_distribution<float> speed_scaled_jitter_dist(-max_jitter_angle_degrees, max_jitter_angle_degrees);
            const float random_jitter_degrees = speed_scaled_jitter_dist(this->rng_);
            const float random_jitter_radians = sf::degrees(random_jitter_degrees).asRadians();
            const float cosine_jitter = std::cos(random_jitter_radians);
            const float sine_jitter = std::sin(random_jitter_radians);
            const sf::Vector2f original_velocity = this->velocity_;
            this->velocity_.x = original_velocity.x * cosine_jitter - original_velocity.y * sine_jitter;
            this->velocity_.y = original_velocity.x * sine_jitter + original_velocity.y * cosine_jitter;
            this->sprite_.rotate(sf::degrees(random_jitter_degrees));
            // SPDLOG_DEBUG("Collision above minimum bounce speed, current speed '{}' results in a speed ratio of '{}'; now bouncing back with a random angle of '{}' degrees!", current_speed, speed_ratio, random_jitter_degrees);
        }
    }

    // Store last legal position for next frame
    this->last_position_ = this->sprite_.getPosition();
}

}  // namespace core::game
