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
#ifndef NDEBUG  // Debug, remove later
#include <imgui.h>
#endif

#include "game.hpp"

namespace core::game {

Track::Track(const Textures tiles,
             std::mt19937 &rng,
             const TrackConfig &config)
    : tiles_(tiles),  //  Copy the small struct to prevent segfaults
      rng_(rng),
      config_(this->validate_config(config)),
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
    const TrackConfig validated_config = this->validate_config(config);
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
    const TrackConfig default_config = this->validate_config(TrackConfig{});  // Validate default compile-time config anyway
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

TrackConfig Track::validate_config(const TrackConfig &config) const
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
    const auto process_edge_down = [&](float main_x,
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
    const auto process_edge_up = [&](float main_x,
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

BaseCar::BaseCar(const sf::Texture &texture,
                 std::mt19937 &rng,
                 const Track &track,
                 const CarConfig &config)
    : sprite_(texture),
      track_(track),
      config_(config),
      rng_(rng),
      // Private
      last_position_(this->track_.get_finish_position()),  // Get spawn point from the track
      velocity_(0.0f, 0.0f),
      is_accelerating_(false),
      is_braking_(false),
      is_steering_left_(false),
      is_steering_right_(false),
      is_handbraking_(false),
      steering_wheel_angle_(0.0f)
{
    // Center the sprite origin for correct rotation and positioning
    this->sprite_.setOrigin(this->sprite_.getLocalBounds().getCenter());
    // Set initial position of the car sprite based on the spawn point from the track, as set in the initializer list
    this->sprite_.setPosition(this->last_position_);
}

void BaseCar::reset()
{
    const sf::Vector2f spawn_point = this->track_.get_finish_position();
    this->sprite_.setPosition(spawn_point);
    this->sprite_.setRotation(sf::degrees(0.0f));
    this->last_position_ = spawn_point;
    this->velocity_ = {0.0f, 0.0f};
    this->steering_wheel_angle_ = 0.0f;
}

[[nodiscard]] sf::Vector2f BaseCar::get_position() const
{
    return this->sprite_.getPosition();
}

[[nodiscard]] sf::Vector2f BaseCar::get_velocity() const
{
    return this->velocity_;
}

[[nodiscard]] float BaseCar::get_speed() const
{
    return std::hypot(this->velocity_.x, this->velocity_.y);
}

[[nodiscard]] float BaseCar::get_steering_angle() const
{
    return this->steering_wheel_angle_;
}

void BaseCar::gas()
{
    this->is_accelerating_ = true;
}

void BaseCar::brake()
{
    this->is_braking_ = true;
}

void BaseCar::steer_left()
{
    this->is_steering_left_ = true;
}

void BaseCar::steer_right()
{
    this->is_steering_right_ = true;
}

void BaseCar::handbrake()
{
    this->is_handbraking_ = true;
}

void BaseCar::update(const float dt)
{
    this->apply_physics_step(dt);
}

void BaseCar::draw(sf::RenderTarget &target) const
{
    target.draw(this->sprite_);
}

void BaseCar::apply_physics_step(const float dt)
{
    // Cancel opposite steering inputs
    if (this->is_steering_left_ && this->is_steering_right_) {
        this->is_steering_left_ = false;
        this->is_steering_right_ = false;
    }

    // Compute forward unit vector from current heading
    const float heading_angle_radians = this->sprite_.getRotation().asRadians();
    const sf::Vector2f forward_unit_vector = {std::cos(heading_angle_radians), std::sin(heading_angle_radians)};

    // Storage for current speed
    float current_speed = this->get_speed();

    // Apply gas throttle along forward axis
    if (this->is_accelerating_) {
        this->velocity_ += forward_unit_vector * (this->config_.throttle_acceleration_rate_pixels_per_second_squared * dt);
        current_speed = this->get_speed();
    }

    // Apply foot brake deceleration
    if (this->is_braking_ && current_speed > this->config_.stopped_speed_threshold_pixels_per_second) {
        const float brake_reduction = std::min(this->config_.brake_deceleration_rate_pixels_per_second_squared * dt, current_speed);
        const sf::Vector2f velocity_unit_vector = this->velocity_ / current_speed;
        this->velocity_ -= velocity_unit_vector * brake_reduction;
        current_speed -= brake_reduction;
    }

    // Apply handbrake deceleration
    if (this->is_handbraking_ && current_speed > this->config_.stopped_speed_threshold_pixels_per_second) {
        const float new_speed = current_speed - this->config_.handbrake_deceleration_rate_pixels_per_second_squared * dt;
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
    if (!this->is_accelerating_ && !this->is_braking_ && !this->is_handbraking_ && current_speed > this->config_.stopped_speed_threshold_pixels_per_second) {
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

    // Update steering wheel angle from input
    if (this->is_steering_left_) {
        this->steering_wheel_angle_ -= this->config_.steering_turn_rate_degrees_per_second * dt;
    }
    if (this->is_steering_right_) {
        this->steering_wheel_angle_ += this->config_.steering_turn_rate_degrees_per_second * dt;
    }

    // Auto center steering wheel when no steering input is active
    if (!this->is_steering_left_ && !this->is_steering_right_) {
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

    // Reset control flags for next frame
    this->is_accelerating_ = false;
    this->is_braking_ = false;
    this->is_steering_left_ = false;
    this->is_steering_right_ = false;
    this->is_handbraking_ = false;
}

AICar::AICar(const sf::Texture &texture,
             std::mt19937 &rng,
             const Track &track,
             const CarConfig &config)
    : BaseCar(texture, rng, track, config),
      current_waypoint_index_number_(1)  // Start at waypoint index 1 (waypoint 0 is the spawn/finish point)
{
}

void AICar::reset()
{
    // Call base class reset to handle sprite and physics
    BaseCar::reset();

    // Reset waypoint index to 1 (skip spawn point at index 0)
    this->current_waypoint_index_number_ = 1;
}

void AICar::update(const float dt)
{
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

    // Calculate distances
    const sf::Vector2f vector_to_current_waypoint = current_waypoint.position - car_position;
    const float distance_to_current_waypoint = std::hypot(vector_to_current_waypoint.x, vector_to_current_waypoint.y);
    const float waypoint_reach_distance = tile_size * this->waypoint_reach_factor_;

    // Simple collision detection - just check one point ahead
    bool collision_detected = false;
    const sf::Vector2f car_velocity = this->get_velocity();
    const float velocity_magnitude = std::hypot(car_velocity.x, car_velocity.y);
    if (velocity_magnitude > 10.0f) {
        const sf::Vector2f velocity_normalized = car_velocity / velocity_magnitude;
        const sf::Vector2f check_point = car_position + velocity_normalized * (tile_size * this->collision_distance_);
        collision_detected = !this->track_.is_on_track(check_point);
    }

    // Simple steering logic with early corner turning
    const float desired_heading_radians = std::atan2(vector_to_current_waypoint.y, vector_to_current_waypoint.x);
    const float current_heading_radians = this->sprite_.getRotation().asRadians();
    const float heading_difference_radians = std::remainder(desired_heading_radians - current_heading_radians, 2.0f * std::numbers::pi_v<float>);

    // Look ahead for early corner turning - check if next waypoint is a corner
    const bool approaching_corner = (next_waypoint.type == TrackWaypoint::DrivingType::Corner);
    const bool at_corner = (current_waypoint.type == TrackWaypoint::DrivingType::Corner);

    // Use more aggressive steering when approaching or at corners with slight randomness
    float steering_threshold;
    if (at_corner || approaching_corner) {
        steering_threshold = this->corner_steering_threshold_ * this->get_random_variation();
    }
    else {
        steering_threshold = this->straight_steering_threshold_ * this->get_random_variation();
    }

    // Early corner turning: if approaching corner and close enough, use corner threshold with randomness
    if (approaching_corner && distance_to_current_waypoint < tile_size * this->early_corner_turn_distance_ * this->get_random_variation()) {
        steering_threshold = this->corner_steering_threshold_ * this->get_random_variation();
    }

    // Only steer if we need to turn significantly or there's a collision
    const bool should_steer = collision_detected || std::abs(heading_difference_radians) > steering_threshold;

    // Add steering smoothing to reduce wiggling - require minimum heading difference for straights
    const bool is_on_straight = !at_corner && !approaching_corner;
    const float minimum_steering_difference = is_on_straight ? this->minimum_straight_steering_difference_ : 0.001f;

    if (should_steer && std::abs(heading_difference_radians) > minimum_steering_difference) {
        if (heading_difference_radians < 0.0f) {
            this->steer_left();
        }
        else {
            this->steer_right();
        }
    }

    // Speed management with slight randomness to create variety
    const float base_target_speed =
        (current_waypoint.type == TrackWaypoint::DrivingType::Corner || next_waypoint.type == TrackWaypoint::DrivingType::Corner)
            ? tile_size * this->corner_speed_factor_
            : tile_size * this->straight_speed_factor_;

    const float target_speed = base_target_speed * this->get_random_variation();
    const float brake_distance = tile_size * this->brake_distance_factor_ * this->get_random_variation();

    // More intelligent braking logic with randomness
    const bool approaching_corner_too_fast = (next_waypoint.type == TrackWaypoint::DrivingType::Corner) &&
                                             (distance_to_current_waypoint < brake_distance) &&
                                             (current_speed > target_speed * (1.5f * this->get_random_variation()));

    const bool speed_too_high = current_speed > target_speed * (2.0f * this->get_random_variation());  // Only brake if way too fast

    const bool should_brake = collision_detected || speed_too_high || approaching_corner_too_fast;

    if (should_brake) {
        this->brake();
    }
    else if (current_speed < target_speed * (0.8f * this->get_random_variation())) {  // Start accelerating sooner
        this->gas();
    }

    // Advance waypoint with slight randomness in reach distance
    const float randomized_waypoint_reach_distance = waypoint_reach_distance * this->get_random_variation();
    if (distance_to_current_waypoint < randomized_waypoint_reach_distance) {
        this->current_waypoint_index_number_ = next_index;
    }

#ifndef NDEBUG
    // Runtime-configurable debug window
    const ImGuiViewport *vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + 10.0f, vp->WorkPos.y + 10.0f), ImGuiCond_Always, ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImVec2(450.0f, 550.0f), ImGuiCond_Always);

    if (ImGui::Begin("AI", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus |
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoFocusOnAppearing)) {

        // Telemetry section
        if (ImGui::CollapsingHeader("Telemetry", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();

            // Speed info
            ImGui::Text("Speed: %.0f px/s", static_cast<double>(current_speed));
            ImGui::Text("Target Speed: %.0f px/s", static_cast<double>(target_speed));

            ImGui::Separator();

            // Distance info
            ImGui::Text("Distance to Waypoint: %.0f px", static_cast<double>(distance_to_current_waypoint));
            ImGui::Text("Brake Distance: %.0f px", static_cast<double>(brake_distance));

            ImGui::Separator();

            // Steering info
            ImGui::Text("Heading Diff: %.3f rad", static_cast<double>(heading_difference_radians));

            ImGui::Separator();

            // Status info
            ImGui::Text("Current Type: %s", current_waypoint.type == TrackWaypoint::DrivingType::Corner ? "Corner" : "Straight");
            ImGui::Text("Next Type: %s", next_waypoint.type == TrackWaypoint::DrivingType::Corner ? "Corner" : "Straight");

            // Status indicators
            ImGui::Text("Collision: %s", collision_detected ? "YES" : "NO");
            ImGui::Text("Should Brake: %s", should_brake ? "YES" : "NO");
            ImGui::Text("Approaching Corner Fast: %s", approaching_corner_too_fast ? "YES" : "NO");
            ImGui::Text("Speed Too High: %s", speed_too_high ? "YES" : "NO");
            ImGui::Text("Approaching Corner: %s", approaching_corner ? "YES" : "NO");
            ImGui::Text("At Corner: %s", at_corner ? "YES" : "NO");

            ImGui::Unindent();
        }

        // AI Settings section
        if (ImGui::CollapsingHeader("Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent();

            // Basic settings
            ImGui::SeparatorText("Basic Parameters:");
            ImGui::PushItemWidth(-180.0f);  // Make sliders narrower to fit window

            ImGui::SliderFloat("Waypoint Reach", &this->waypoint_reach_factor_, 0.1f, 2.0f, "%.2fx");
            ImGui::TextWrapped("How close the car must get to a waypoint before targeting the next one. Lower = more precise but may miss waypoints.");

            ImGui::SliderFloat("Collision Distance", &this->collision_distance_, 0.1f, 2.0f, "%.2fx");
            ImGui::TextWrapped("How far ahead the car looks for track boundaries to avoid crashes. Higher = earlier collision avoidance.");

            // Steering settings
            ImGui::SeparatorText("Steering Parameters");

            ImGui::SliderFloat("Straight Threshold", &this->straight_steering_threshold_, 0.05f, 1.57f, "%.3f rad");
            ImGui::TextWrapped("How much the car heading must differ from target direction before steering on straights. Higher = less wiggling but slower corrections.");

            ImGui::SliderFloat("Corner Threshold", &this->corner_steering_threshold_, 0.001f, 0.2f, "%.3f rad");
            ImGui::TextWrapped("How much the car heading must differ from target direction before steering in corners. Lower = more responsive turning.");

            ImGui::SliderFloat("Min Straight Steer", &this->minimum_straight_steering_difference_, 0.01f, 0.1f, "%.3f rad");
            ImGui::TextWrapped("Minimum heading difference required to steer on straights. Prevents tiny steering corrections that cause wiggling.");

            ImGui::SliderFloat("Early Corner Turn", &this->early_corner_turn_distance_, 0.5f, 3.0f, "%.2fx");
            ImGui::TextWrapped("How far before a corner the car starts using corner steering settings. Higher = starts turning sooner for smoother cornering.");

            // Speed settings
            ImGui::SeparatorText("Speed Parameters");

            ImGui::SliderFloat("Corner Speed", &this->corner_speed_factor_, 0.3f, 3.0f, "%.2fx");
            ImGui::TextWrapped("Target speed multiplier for corners. Lower = slower and safer cornering, higher = faster but riskier.");

            ImGui::SliderFloat("Straight Speed", &this->straight_speed_factor_, 0.5f, 4.0f, "%.2fx");
            ImGui::TextWrapped("Target speed multiplier for straight sections. Higher = faster top speed on straights.");

            ImGui::SliderFloat("Brake Distance", &this->brake_distance_factor_, 0.1f, 3.0f, "%.2fx");
            ImGui::TextWrapped("How far before corners the car starts braking. Higher = earlier braking for safer cornering.");

            ImGui::PopItemWidth();

            ImGui::Spacing();

            // Reset button
            if (ImGui::Button("Reset to Defaults", ImVec2(-1.0f, 0.0f))) {
                this->waypoint_reach_factor_ = 0.75f;
                this->collision_distance_ = 0.65f;
                this->straight_steering_threshold_ = 0.25f;
                this->corner_steering_threshold_ = 0.08f;
                this->minimum_straight_steering_difference_ = 0.1f;
                this->early_corner_turn_distance_ = 1.0f;
                this->corner_speed_factor_ = 1.2f;
                this->straight_speed_factor_ = 3.0f;
                this->brake_distance_factor_ = 3.0f;
            }

            ImGui::Unindent();
        }
    }
    ImGui::End();
#endif

    // Apply physics
    BaseCar::update(dt);
}

[[nodiscard]] float AICar::get_random_variation() const
{
    std::uniform_real_distribution<float> distribution(this->random_variation_minimum_, this->random_variation_maximum_);
    return distribution(this->rng_);
}

}  // namespace core::game
