/**
 * @file entities.cpp
 */

#include <algorithm>  // for std::clamp, std::max, std::min
#include <cmath>      // for std::atan2, std::hypot, std::remainder, std::copysign, std::cos, std::sin, std::lerp
#include <cstddef>    // for std::size_t
#include <cstdlib>    // for std::abs
#include <numbers>    // for std::numbers
#include <random>     // for std::mt19937, std::uniform_real_distribution

#include <SFML/Graphics.hpp>
#include <spdlog/spdlog.h>

#include "core/world.hpp"  // We depend on the Track class for car collision detection and waypoints
#include "entities.hpp"

namespace game::entities {

Car::Car(const sf::Texture &texture,
         std::mt19937 &rng,
         const core::world::Track &track,
         const CarControlMode control_mode,
         const CarConfig &config)
    : sprite_(texture),
      shadow_sprite_(texture),
      track_(track),
      config_(config),
      rng_(rng),
      control_mode_(control_mode),
      last_position_({0.0f, 0.0f}),
      velocity_({0.0f, 0.0f}),
      current_input_(),
      steering_wheel_angle_(0.0f),
      current_waypoint_index_number_(1),
      drift_score_(0.0f),
      current_lateral_slip_velocity_(0.0f),
      just_hit_wall_(false),
      last_wall_hit_speed_(0.0f),
      ai_update_timer_(0.0f)
{
    this->sprite_.setOrigin(this->sprite_.getLocalBounds().getCenter());

    // Setup shadow sprite
    this->shadow_sprite_.setOrigin(this->shadow_sprite_.getLocalBounds().getCenter());
    this->shadow_sprite_.setColor({0, 0, 0, 80});  // Semi-transparent black shadow
    this->shadow_sprite_.setScale({0.9f, 0.9f});   // Slightly smaller than the car

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
    this->ai_update_timer_ = 0.0f;

    // Reset drift score
    this->drift_score_ = 0.0f;

    // Reset lateral slip velocity
    this->current_lateral_slip_velocity_ = 0.0f;

    // Reset collision state
    this->just_hit_wall_ = false;
    this->last_wall_hit_speed_ = 0.0f;
}

[[nodiscard]] CarState Car::get_state() const
{
    // Calculate heading angle from sprite rotation
    const float heading_radians = this->sprite_.getRotation().asRadians();

    return CarState{
        .position = this->sprite_.getPosition(),
        .velocity = this->velocity_,
        .speed = std::hypot(this->velocity_.x, this->velocity_.y),
        .heading_radians = heading_radians,
        .lateral_slip_velocity = this->current_lateral_slip_velocity_,
        .steering_angle = this->steering_wheel_angle_,
        .control_mode = this->control_mode_,
        .waypoint_index = this->current_waypoint_index_number_,
        .drift_score = this->drift_score_,
        .just_hit_wall = this->just_hit_wall_,
        .last_wall_hit_speed = this->last_wall_hit_speed_};
}

void Car::set_control_mode(const CarControlMode control_mode)
{
    this->control_mode_ = control_mode;
}

void Car::apply_input(const CarInput &input)
{
    // Only store input values when in Player mode, ignore in AI mode
    if (this->control_mode_ == CarControlMode::Player) {
        this->current_input_ = input;
        // Uncomment when debugging controller input or whatever
        // SPDLOG_DEBUG("Applying input: throttle = {:.2f}, brake = {:.2f}, steering = {:.2f}, handbrake = {:.2f}",
        //              input.throttle, input.brake, input.steering, input.handbrake);
    }
}

void Car::update(const float dt)
{
    // Update waypoint tracking for all cars to maintain race position
    this->update_waypoint_tracking();

    // Handle AI behavior if in AI mode
    if (this->control_mode_ == CarControlMode::AI) {
        this->update_ai_behavior(dt);
    }

    // Apply physics regardless of control mode
    this->apply_physics_step(dt);

    // Update shadow sprite position and rotation to match main sprite (with offset)
    this->shadow_sprite_.setPosition({this->sprite_.getPosition().x + 10.0f,
                                      this->sprite_.getPosition().y + 10.0f});
    this->shadow_sprite_.setRotation(this->sprite_.getRotation());
}

void Car::draw(sf::RenderTarget &target) const
{
    // Draw shadow first (so it appears behind the car)
    target.draw(this->shadow_sprite_);

    // Draw the actual car on top of the shadow
    target.draw(this->sprite_);
}

void Car::update_ai_behavior(const float dt)
{
    // Accumulate the delta time
    this->ai_update_timer_ += dt;

    // If the accumulated time does not the update rate, ignore to save performance
    if (this->ai_update_timer_ < this->ai_update_rate) {
        return;
    }

    // Reset timer for next AI update cycle
    this->ai_update_timer_ -= this->ai_update_rate;  // Keep any overshoot

    // AI behavior constants
    static constexpr float collision_distance = 0.65f;                           // Distance ahead to check for collisions as fraction of tile size; increase = avoid collisions earlier, decrease = check collisions closer to car
    static constexpr float straight_steering_threshold = 0.25f;                  // Heading difference threshold for steering on straights in radians; increase = less sensitive steering on straights, decrease = more twitchy steering on straights
    static constexpr float corner_steering_threshold = 0.08f;                    // Heading difference threshold for steering in corners in radians; increase = less responsive cornering, decrease = more aggressive cornering
    static constexpr float minimum_straight_steering_difference = 0.1f;          // Minimum heading difference for straight steering in radians; increase = reduce steering wobble but less precision, decrease = more precise but potentially jittery steering
    static constexpr float early_corner_turn_distance = 1.0f;                    // Distance factor for early corner turning; increase = start turning earlier before corners, decrease = turn later and sharper into corners
    static constexpr float corner_speed_factor = 1.2f;                           // Speed multiplier for corners as fraction of tile size; increase = faster through corners but riskier, decrease = slower and safer through corners
    static constexpr float straight_speed_factor = 3.0f;                         // Speed multiplier for straights as fraction of tile size; increase = faster on straights, decrease = slower and more conservative on straights
    static constexpr float brake_distance_factor = 3.0f;                         // Distance factor for braking before corners; increase = start braking earlier before corners, decrease = brake later and more aggressively
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
    const core::world::TrackWaypoint &current_waypoint = waypoints[current_index];
    const core::world::TrackWaypoint &next_waypoint = waypoints[next_index];
    const sf::Vector2f car_position = this->sprite_.getPosition();
    const float tile_size = static_cast<float>(this->track_.get_config().size_px);
    const float current_speed = std::hypot(this->velocity_.x, this->velocity_.y);

    // Cache random variations to avoid multiple RNG calls per parameter
    std::uniform_real_distribution<float> random_distribution(random_variation_minimum_, random_variation_maximum_);
    const float speed_random_variation = random_distribution(this->rng_);
    const float steering_random_variation = random_distribution(this->rng_);
    const float distance_random_variation = random_distribution(this->rng_);

    // Calculate distances
    const sf::Vector2f vector_to_current_waypoint = current_waypoint.position - car_position;
    const float distance_to_current_waypoint = std::hypot(vector_to_current_waypoint.x, vector_to_current_waypoint.y);

    // Collision detection - check one point ahead based on velocity direction
    bool collision_detected = false;
    const sf::Vector2f car_velocity = this->velocity_;
    const float velocity_magnitude = std::hypot(car_velocity.x, car_velocity.y);
    const float collision_velocity_threshold = tile_size * collision_velocity_threshold_factor;
    if (velocity_magnitude > collision_velocity_threshold) {
        const sf::Vector2f velocity_normalized = car_velocity / velocity_magnitude;
        const sf::Vector2f check_point = car_position + velocity_normalized * (tile_size * collision_distance);
        collision_detected = !this->track_.is_on_track(check_point);
    }

    // Determine driving context for better decision making
    const bool at_corner = (current_waypoint.type == core::world::TrackWaypoint::DrivingType::Corner);
    const bool approaching_corner = (next_waypoint.type == core::world::TrackWaypoint::DrivingType::Corner);
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
}

void Car::apply_physics_step(const float dt)
{
    // Reset collision state at the beginning of each physics step
    this->just_hit_wall_ = false;
    this->last_wall_hit_speed_ = 0.0f;

    // Compute forward unit vector from current heading
    const float heading_angle_radians = this->sprite_.getRotation().asRadians();
    const sf::Vector2f forward_unit_vector = {std::cos(heading_angle_radians), std::sin(heading_angle_radians)};

    // Storage for current speed
    float current_speed = std::hypot(this->velocity_.x, this->velocity_.y);

    // Apply gas throttle along forward axis (using analog input)
    if (this->current_input_.throttle > 0.0f) {
        const float throttle_force = this->current_input_.throttle * this->config_.throttle_acceleration_rate_pixels_per_second_squared * dt;
        this->velocity_ += forward_unit_vector * throttle_force;
        current_speed = std::hypot(this->velocity_.x, this->velocity_.y);
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
        if (new_speed < this->config_.stopped_speed_threshold_pixels_per_second) {
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

    // Store lateral slip velocity for use in get_state() to avoid recalculation
    this->current_lateral_slip_velocity_ = lateral_speed;
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
        // Record collision state
        this->just_hit_wall_ = true;
        this->last_wall_hit_speed_ = current_speed;

        this->sprite_.setPosition(this->last_position_);
        // Restore last legal position
        this->velocity_ = -this->velocity_ * this->config_.collision_velocity_retention_ratio;
        current_speed = std::hypot(this->velocity_.x, this->velocity_.y);
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

void Car::update_waypoint_tracking()
{
    // Get basic waypoint info
    const auto &waypoints = this->track_.get_waypoints();

    // Safety check for empty waypoints
    if (waypoints.empty()) {
        SPDLOG_WARN("No waypoints available, cannot update waypoint tracking!");
        return;
    }

    // Calculate distances and waypoint reach logic
    const std::size_t current_index = this->current_waypoint_index_number_;
    const std::size_t next_index = (current_index + 1) % waypoints.size();
    const core::world::TrackWaypoint &current_waypoint = waypoints[current_index];
    const sf::Vector2f car_position = this->sprite_.getPosition();
    const float tile_size = static_cast<float>(this->track_.get_config().size_px);

    // Calculate distance to current waypoint
    const sf::Vector2f vector_to_current_waypoint = current_waypoint.position - car_position;
    const float distance_to_current_waypoint = std::hypot(vector_to_current_waypoint.x, vector_to_current_waypoint.y);
    const float waypoint_reach_distance = tile_size * waypoint_reach_factor_;

    // Apply random variation to waypoint reach distance for more natural behavior
    std::uniform_real_distribution<float> random_distribution(random_variation_minimum_, random_variation_maximum_);
    const float distance_random_variation = random_distribution(this->rng_);
    const float randomized_waypoint_reach_distance = waypoint_reach_distance * distance_random_variation;

    // Advance waypoint if close enough
    if (distance_to_current_waypoint < randomized_waypoint_reach_distance) {
        this->current_waypoint_index_number_ = next_index;
    }
}

}  // namespace game::entities
