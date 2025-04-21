/**
 * @file game.hpp
 *
 * @brief Game world map abstractions.
 */

#pragma once

#include <algorithm>  // for std::clamp, std::min, std::max
#include <cmath>      // for std::hypot, std::remainder, std::atan2, std::sin, std::cos
#include <cstddef>    // for std::size_t
#include <numbers>    // for std::numbers
#include <random>     // for std::mt19937
#include <vector>     // for std::vector

#include <SFML/Graphics.hpp>

namespace core::game {

/**
 * @brief Units of measurement + constexpr conversion functions.
 */
namespace units {

/**
 * @brief Default pixel-to-meter conversion factor.
 *
 * This is roughly modeled after the Nissan Silvia S14 real-world dimensions ({4.5f, 1.7f}) vs. in-game player car sprite (car_black_1).
 */
inline constexpr float PX_TO_M = 0.028f;

/**
 * @brief Default pixel-to-kilometer per hour conversion factor.
 *
 * This is roughly modeled after the Nissan Silvia S14 real-world dimensions ({4.5f, 1.7f}) vs. in-game player car sprite (car_black_1).
 */
inline constexpr float PX_TO_KPH = 0.1008f;

/**
 * @brief Convert from px/s to km/h.
 *
 * @param px_per_s Speed in pixels per second (e.g., "100.f").
 *
 * @return Speed in kilometers per hour (e.g., "360.f")
 */
[[nodiscard]] constexpr float px_per_s_to_kph(const float px_per_s)
{
    return px_per_s * PX_TO_KPH;
}

/**
 * @brief Convert from kp/h to px/s.
 *
 * @param kph Speed in kilometers per hour (e.g., "360.f").
 *
 * @return Speed in pixels per second (e.g., "100.f")
 */
[[nodiscard]] constexpr float kph_to_px_per_s(const float kph)
{
    return kph / PX_TO_KPH;
}

/**
 * @brief Convert from px to meters.
 *
 * @param px Distance in pixels (e.g., "100.f").
 *
 * @return Distance in meters (e.g., "2.8f").
 */
[[nodiscard]] constexpr float px_to_m(const float px)
{
    return px * PX_TO_M;
}

/**
 * @brief Convert from meters to px.
 *
 * @param meters Distance in meters (e.g., "2.8f").
 *
 * @return Distance in pixels (e.g., "100.f").
 */
[[nodiscard]] constexpr float m_to_px(const float meters)
{
    return meters / PX_TO_M;
}

}  // namespace units

/**
 * @brief Struct that represents the configurable parameters of the track.
 *
 * @note This struct is marked as "final" to prevent inheritance.
 */
struct TrackConfig final {
    /**
     * @brief Number of horizontal tiles, i.e., width (e.g., "8").
     *
     * @note If detours are enabled, +1 tile may be added to each side when a detour occurs, increasing the effective total width, even though the core "horizontal_count" remains unchanged
     */
    std::size_t horizontal_count = 7;

    /**
     * @brief Number of vertical tiles, i.e., height (e.g., "6").
     */
    std::size_t vertical_count = 12;

    /**
     * @brief Size of each tile in pixels (e.g., "256").
     *
     * @note This does not depend on the size of the texture; it will be scaled to this size.
     *
     * @details The default texture size is 128x128px, so we are scaling it up 4 times.
     */
    std::size_t size_px = 512;

    /**
     * @brief Chance for a detour to occur [0.0, 1.0] for detours (e.g., "0.1" for 10%).
     *
     * @note Set to "0.0" for no detours.
     */
    float detour_chance_pct = 0.4f;

    // /**
    //  * @brief Default three-way comparison operator.
    //  *
    //  * This automatically generates "operator==", "operator<", "operator<=", "operator>", and "operator>=".
    //  */
    // [[nodiscard]] auto operator<=>(const TrackConfig &) const = default;

    // Comparing floats might be a bad idea,
};

/**
 * @brief Struct that represents a waypoint on the track, i.e., position and type.
 *
 * @note This struct is marked as "final" to prevent inheritance.
 */
struct TrackWaypoint final {
    /**
     * @brief Enum that represents the type of a waypoint.
     */
    enum class Type {
        /**
         * @brief Straight line. Can go fast.
         */
        Straight,

        /**
         * @brief Corner. Need to slow down, preferably before the corner.
         */
        Corner
    };

    /**
     * @brief Position of the waypoint (e.g., "{100.f, 200.f}").
     */
    sf::Vector2f position;

    /**
     * @brief Type of the waypoint (e.g., "Straight" or "Corner").
     */
    Type type;
};

/**
 * @brief Class that manages the race track, including its textures, configuration, positioning, and rendering.
 *
 * On construction, the class builds the track using the provided textures and config.
 *
 * @note This class is marked as "final" to prevent inheritance.
 */
class Track final {
  public:
    /**
     * @brief Parameter struct for the track textures. Holds references to the textures used to build the track.
     *
     * The caller is responsible for ensuring that these textures remain valid for the lifetime of the "Track" instance.
     * It is assumed that all textures are square and of the same size (e.g., 256x256) for uniform scaling.
     *
     * @note This struct is marked as "final" to prevent inheritance.
     */
    struct Textures final {
        /**
         * @brief Top-left curve texture.
         *
         * In ASCII:
         * ```
         * XXX
         * X
         * X
         * ```
         */
        const sf::Texture &top_left;

        /**
         * @brief Top-right curve texture.
         *
         * In ASCII:
         * ```
         * XXX
         *   X
         *   X
         */
        const sf::Texture &top_right;

        /**
         * @brief Bottom-right curve texture.
         *
         * In ASCII:
         * ```
         *   X
         *   X
         * XXX
         */
        const sf::Texture &bottom_right;

        /**
         * @brief Bottom-left curve texture.
         *
         * In ASCII:
         * ```
         * X
         * X
         * XXX
         */
        const sf::Texture &bottom_left;

        /**
         * @brief [┃] Vertical road texture.
         *
         * In ASCII:
         * ```
         *  X
         *  X
         *  X
         */
        const sf::Texture &vertical;

        /**
         * @brief [━] Horizontal road texture.
         *
         * In ASCII:
         * ```
         *
         * XXX
         *
         */
        const sf::Texture &horizontal;

        /**
         * @brief [━] Horizontal finish line texture.
         *
         * In ASCII:
         * ```
         *
         * XXX
         *
         */
        const sf::Texture &horizontal_finish;
    };
    /**
     * @brief Construct a new Track object.
     *
     * On construction, the track is built using the provided textures and config.
     *
     * @param tiles Tiles struct containing the textures. It is assumed that all textures are square (e.g., 256x256) for uniform scaling. The caller is responsible for ensuring that these textures remain valid for the lifetime of the Track.
     * @param rng Instance of a random number generator (e.g., std::mt19937) used for generating random detours.
     * @param config Configuration struct containing the track configuration (default: "TrackConfig()").
     */
    explicit Track(const Textures &tiles,
                   std::mt19937 &rng,
                   const TrackConfig &config = TrackConfig());  // Use default config

    /**
     * @brief Set the configuration of the track and rebuild it.
     *
     * @param config New configuration for the track.
     */
    void set_config(const TrackConfig &config);

    /**
     * @brief Get the current configuration of the track.
     *
     * @return Reference to the current configuration.
     */
    [[nodiscard]] const TrackConfig &get_config() const;

    /**
     * @brief Get the finish point of the track.
     *
     * This is the position of the finish line tile.
     *
     * @return Finish point of the track.
     */
    [[nodiscard]] const sf::Vector2f &get_finish_point() const;

    /**
     * @brief Check if a given world position is within the track.
     *
     * This is a simple check that treats every tile as a rectangle, regardless of its actual shape.
     *
     * @param world_position Position in world coordinates to check.
     *
     * @return True if the position is on the track, false otherwise.
     */
    [[nodiscard]] bool is_on_track(const sf::Vector2f &world_position) const;

    /**
     * @brief Get the waypoints on the track.
     *
     * This is used for AI navigation.
     *
     * @return Vector of waypoints on the track - positions and types.
     */
    [[nodiscard]] const std::vector<TrackWaypoint> &get_waypoints() const;

    /**
     * @brief Draw the track on the provided render target.
     *
     * @param target Target window where the track will be drawn.
     */
    void draw(sf::RenderTarget &target) const;

  private:
    /**
     * @brief Build the track using the current configuration and textures.
     *
     * Random detours are added for the left and right edges of the track based on the configuration.
     *
     * @note This is marked as private, because we only want to build the track on construction and explicit config changes.
     */
    void build();

    /**
     * @brief Lightweight struct holding references to texture tiles.
     */
    const Textures &tiles_;

    /**
     * @brief Random number generator.
     */
    std::mt19937 &rng_;

    /**
     * @brief Configuration of the track.
     */
    TrackConfig config_;

    /**
     * @brief Vector of sprites representing the track tiles.
     *
     * @note These are the actual objects displayed on screen.
     */
    std::vector<sf::Sprite> sprites_;

    /**
     * @brief Vector of waypoints on the track.
     *
     * @note This is used for AI navigation.
     */
    std::vector<TrackWaypoint> waypoints_;

    /**
     * @brief Collision bounds for the track, based on the sprites.
     *
     * @note This is used for collision detection.
     */
    std::vector<sf::FloatRect> collision_bounds_;

    /**
     * @brief Finish point of the track.
     *
     * This is the position of the finish line tile. It can be used as a spawn point for cars.
     */
    sf::Vector2f finish_point_;
};

struct CarConfig final {
    /// Throttle acceleration in pixels per second squared
    float forward_acceleration_pixels_per_second_squared = 700.0f;

    /// Brake deceleration in pixels per second squared
    float brake_deceleration_pixels_per_second_squared = 950.0f;

    /// Handbrake deceleration in pixels per second squared
    float handbrake_deceleration_pixels_per_second_squared = 2200.0f;

    /// Passive engine braking in pixels per second squared
    float engine_braking_pixels_per_second_squared = 80.0f;

    /// Maximum allowed speed in pixels per second
    float maximum_speed_pixels_per_second = 2500.0f;

    /// Steering turn rate in degrees per second when input held
    float steering_turn_rate_degrees_per_second = 520.0f;

    /// Steering centering rate in degrees per second when releasing input
    float steering_centering_rate_degrees_per_second = 580.0f;

    /// Maximum steering wheel angle in degrees
    float maximum_steering_wheel_angle_degrees = 180.0f;

    /// Turn factor at zero speed (full steering responsiveness)
    float turn_factor_at_zero_speed = 1.0f;

    /// Turn factor at full speed (reduced steering responsiveness)
    float turn_factor_at_full_speed = 0.8f;

    /// Side slip damping coefficient per second
    float side_slip_damping_coefficient_per_second = 12.0f;

    /// Speed retention factor on collision bounce (0 = full stop, 1 = keep full speed - it will bounce like a rubber ball, lol)
    float collision_speed_retention_factor = 0.3f;
};

/**
 * @brief Virtual base class for cars: shared physics, collision, rendering.
 *
 * Usage:
 * 1) Construct once before entering the main loop.
 * 2) Call update() each frame with elapsed time.
 * 3) Call draw() each frame to render.
 * 4) Call reset() when respawning to reset state.
 */
class Car {
  public:
    explicit Car(const sf::Texture &texture,
                 const sf::Vector2f &initial_position,
                 std::mt19937 &rng,
                 const Track &track,
                 const CarConfig &config = CarConfig())
        : sprite_(texture),
          rng_(rng),
          track_(track),
          config_(config)
    {
        this->sprite_.setOrigin(this->sprite_.getLocalBounds().getCenter());
        this->sprite_.setPosition(initial_position);
        this->current_heading_angle_ = sf::degrees(0.0f);
        this->current_velocity_vector_ = {0.0f, 0.0f};
        this->current_steering_wheel_angle_degrees_ = 0.0f;
    }

    virtual ~Car() = default;

    // Shared way to reset everything, useful when respawning
    void reset(const sf::Vector2f &position)
    {
        // Sprite
        this->sprite_.setPosition(position);
        this->sprite_.setRotation(sf::degrees(0.0f));

        // Physics
        this->current_heading_angle_ = sf::degrees(0.0f);
        this->current_velocity_vector_ = {0.0f, 0.0f};
        this->current_steering_wheel_angle_degrees_ = 0.0f;
    }

    [[nodiscard]] sf::Vector2f get_position() const
    {
        return this->sprite_.getPosition();
    }

    [[nodiscard]] sf::Vector2f get_velocity() const
    {
        return this->current_velocity_vector_;
    }

    // Generic, to be overridden way to update the car, based on whether it's a player (keyboard input) or AI (waypoints)
    virtual void update(const float dt) = 0;

    void draw(sf::RenderTarget &target) const
    {
        target.draw(this->sprite_);
    }

  protected:
    // Apply throttle acceleration based on heading
    void accelerate(const float dt)
    {
        // Convert heading angle to radians for math functions
        const float heading_radians = this->current_heading_angle_.asRadians();

        // Increase velocity components along heading direction
        this->current_velocity_vector_.x += std::cos(heading_radians) * this->config_.forward_acceleration_pixels_per_second_squared * dt;
        this->current_velocity_vector_.y += std::sin(heading_radians) * this->config_.forward_acceleration_pixels_per_second_squared * dt;
    }

    // Apply brake deceleration opposite to current velocity
    void brake(const float dt)
    {
        // Compute current speed magnitude
        const float speed_magnitude = std::hypot(this->current_velocity_vector_.x, this->current_velocity_vector_.y);

        // Only brake if moving above a tiny threshold
        if (speed_magnitude > 0.01f) {
            // Unit direction of current velocity
            const float unit_velocity_x = this->current_velocity_vector_.x / speed_magnitude;
            const float unit_velocity_y = this->current_velocity_vector_.y / speed_magnitude;
            // Compute speed reduction this frame
            const float speed_reduction_amount = std::min(this->config_.brake_deceleration_pixels_per_second_squared * dt, speed_magnitude);
            // Subtract reduction along velocity direction
            this->current_velocity_vector_.x -= unit_velocity_x * speed_reduction_amount;
            this->current_velocity_vector_.y -= unit_velocity_y * speed_reduction_amount;
        }
    }

    // Apply handbrake deceleration for sharper stops
    void handbrake(const float dt)
    {
        const float speed_magnitude = std::hypot(this->current_velocity_vector_.x, this->current_velocity_vector_.y);
        if (speed_magnitude > 0.01f) {
            const float unit_velocity_x = this->current_velocity_vector_.x / speed_magnitude;
            const float unit_velocity_y = this->current_velocity_vector_.y / speed_magnitude;
            const float new_speed_after_handbrake = speed_magnitude - this->config_.handbrake_deceleration_pixels_per_second_squared * dt;
            if (new_speed_after_handbrake < 0.1f) {
                // If almost stopped, zero velocity
                this->current_velocity_vector_ = {0.0f, 0.0f};
            }
            else {
                // Otherwise reduce speed but keep direction
                this->current_velocity_vector_.x = unit_velocity_x * new_speed_after_handbrake;
                this->current_velocity_vector_.y = unit_velocity_y * new_speed_after_handbrake;
            }
        }
    }

    // Turn steering wheel to the left
    void steer_left(const float dt)
    {
        this->current_steering_wheel_angle_degrees_ -= this->config_.steering_turn_rate_degrees_per_second * dt;
    }

    // Turn steering wheel to the right
    void steer_right(const float dt)
    {
        this->current_steering_wheel_angle_degrees_ += this->config_.steering_turn_rate_degrees_per_second * dt;
    }

    // Shared way to handle physics, to be called by "update()"
    // This maintains velocity over time, reacts to collisions, engine braking, etc.
    // Both player car and AI car need to use the same physics logic, we are not cheating with AI cars!
    // Shared physics update: input processing, velocity clamping, steering behavior, movement, and collision response
    void handle_physics(const float dt)
    {
        // Store last position in case of collision revert
        this->last_position_vector_ = this->sprite_.getPosition();

        // Apply inputs: accelerate, brake, handbrake, steer left/right
        if (this->is_accelerating) {
            this->accelerate(dt);
        }
        if (this->is_braking) {
            this->brake(dt);
        }
        if (this->is_handbraking) {
            this->handbrake(dt);
        }
        if (this->is_steering_left) {
            this->steer_left(dt);
        }
        if (this->is_steering_right) {
            this->steer_right(dt);
        }

        // Determine if any throttle or braking input was given
        const bool any_throttle_or_brake_input_flag =
            this->is_accelerating || this->is_braking || this->is_handbraking;
        // Compute current speed magnitude
        const float current_speed_magnitude = std::hypot(this->current_velocity_vector_.x, this->current_velocity_vector_.y);

        // If no throttle/brake input and still moving, apply engine brake
        if (!any_throttle_or_brake_input_flag && current_speed_magnitude > 0.01f) {
            const float speed_after_engine_brake = current_speed_magnitude - this->config_.engine_braking_pixels_per_second_squared * dt;
            const float clamped_speed_after_engine_brake = std::max(speed_after_engine_brake, 0.0f);
            this->current_velocity_vector_ *= (clamped_speed_after_engine_brake / current_speed_magnitude);
        }

        // Clamp to maximum speed if exceeded
        if (current_speed_magnitude > this->config_.maximum_speed_pixels_per_second) {
            const float speed_clamp_ratio = this->config_.maximum_speed_pixels_per_second / current_speed_magnitude;
            this->current_velocity_vector_ *= speed_clamp_ratio;
        }

        // Compute forward direction unit vector from heading
        const float heading_radians = this->current_heading_angle_.asRadians();
        const sf::Vector2f forward_direction_vector = {std::cos(heading_radians), std::sin(heading_radians)};

        // Decompose velocity into forward and lateral components
        const float forward_speed_component = forward_direction_vector.x * this->current_velocity_vector_.x + forward_direction_vector.y * this->current_velocity_vector_.y;
        const sf::Vector2f forward_velocity_vector = forward_direction_vector * forward_speed_component;
        const sf::Vector2f lateral_velocity_vector = this->current_velocity_vector_ - forward_velocity_vector;

        // Apply side slip damping
        const float side_slip_damping_factor = std::clamp(this->config_.side_slip_damping_coefficient_per_second * dt, 0.0f, 1.0f);
        this->current_velocity_vector_ = forward_velocity_vector + lateral_velocity_vector * (1.0f - side_slip_damping_factor);

        // Center steering wheel if no steering input
        if (!this->is_steering_left && !this->is_steering_right) {
            const float centering_amount = this->config_.steering_centering_rate_degrees_per_second * dt;
            if (this->current_steering_wheel_angle_degrees_ > centering_amount) {
                this->current_steering_wheel_angle_degrees_ -= centering_amount;
            }
            else if (
                this->current_steering_wheel_angle_degrees_ < -centering_amount) {
                this->current_steering_wheel_angle_degrees_ += centering_amount;
            }
            else {
                this->current_steering_wheel_angle_degrees_ = 0.0f;
            }
        }

        // Clamp steering wheel angle to allowed range
        this->current_steering_wheel_angle_degrees_ = std::clamp(this->current_steering_wheel_angle_degrees_, -this->config_.maximum_steering_wheel_angle_degrees, this->config_.maximum_steering_wheel_angle_degrees);

        // Compute turn factor interpolation based on speed ratio
        const float speed_ratio = (current_speed_magnitude > 0.0f) ? (current_speed_magnitude / this->config_.maximum_speed_pixels_per_second) : 0.0f;
        float computed_turn_factor = this->config_.turn_factor_at_zero_speed * (1.0f - speed_ratio) + this->config_.turn_factor_at_full_speed * speed_ratio;
        if (current_speed_magnitude < 5.0f) {
            // Further reduce steering at very low speed
            computed_turn_factor *= (current_speed_magnitude / 5.0f);
        }

        // Update heading based on steering and turn factor
        this->current_heading_angle_ += sf::degrees(this->current_steering_wheel_angle_degrees_ * computed_turn_factor * dt);

        // Apply rotation and move sprite by velocity
        this->sprite_.setRotation(this->current_heading_angle_);
        this->sprite_.move(this->current_velocity_vector_ * dt);

        // Collision check: if off track, revert and bounce back
        if (!this->track_.is_on_track(this->sprite_.getPosition())) {
            this->sprite_.setPosition(this->last_position_vector_);
            this->current_velocity_vector_ = -this->current_velocity_vector_ * this->config_.collision_speed_retention_factor;
        }

        // Reset all input flags after physics step
        this->is_accelerating = false;
        this->is_braking = false;
        this->is_handbraking = false;
        this->is_steering_left = false;
        this->is_steering_right = false;
    }

    // Protected member variables for subclasses
    sf::Sprite sprite_;                           ///< Sprites used for rendering
    std::mt19937 &rng_;                           ///< RNG for physics/AI variation
    const Track &track_;                          ///< Track for collision checks
    CarConfig config_;                            ///< Physics and behavior settings
    sf::Angle current_heading_angle_;             ///< Heading angle in degrees
    sf::Vector2f current_velocity_vector_;        ///< Velocity vector in pixels/s
    float current_steering_wheel_angle_degrees_;  ///< Steering wheel angle

    bool is_accelerating = false;    ///< True if throttle input is active
    bool is_braking = false;         ///< True if brake input is active
    bool is_handbraking = false;     ///< True if handbrake input is active
    bool is_steering_left = false;   ///< True if steering-left input is active
    bool is_steering_right = false;  ///< True if steering-right input is active

    sf::Vector2f last_position_vector_;  ///< Last position before move, for collision revert
};

/**
 * @brief Player‐controlled car class. Inherits shared physics from Car.
 *
 * Before calling update(), set input flags via set_input().
 */
class PlayerCar final : public Car {
  public:
    using Car::Car;

    void set_input(const bool gas_pressed,
                   const bool brake_pressed,
                   const bool steer_left_pressed,
                   const bool steer_right_pressed,
                   const bool handbrake_pressed)
    {
        this->is_accelerating = gas_pressed;
        this->is_braking = brake_pressed;
        this->is_steering_left = steer_left_pressed;
        this->is_steering_right = steer_right_pressed;
        this->is_handbraking = handbrake_pressed;
    }

    void update(const float dt) override
    {
        this->handle_physics(dt);
    }
};

/**
 * @brief AI‐driven car: follows waypoints and slows for corners.
 */
class AICar final : public Car {
  public:
    using Car::Car;

    void update(const float dt) override
    {
        // Tweakable AI navigation parameters
        constexpr float waypoint_reach_distance = 10.0f;
        constexpr float corner_braking_extra_margin = 5.0f;
        constexpr float next_corner_prebraking_extra_margin = 15.0f;
        constexpr float steering_angle_threshold_radians = 0.02f;

        // 1) Retrieve waypoints from the track
        const auto &waypoints = this->track_.get_waypoints();

        // Determine current and next waypoint indices, wrapping if needed
        const std::size_t current_index = this->current_target_waypoint_index_;
        std::size_t next_index = current_index + 1;
        if (next_index >= waypoints.size()) {
            next_index = 1;  // Wrap around, skip spawn point at 0
        }

        const TrackWaypoint &current_waypoint = waypoints[current_index];
        const TrackWaypoint &next_waypoint = waypoints[next_index];

        // 2) Compute vector and distance to current waypoint
        const sf::Vector2f current_position = this->sprite_.getPosition();
        const sf::Vector2f vector_to_current_waypoint = current_waypoint.position - current_position;
        const float distance_to_current_waypoint = std::hypot(vector_to_current_waypoint.x, vector_to_current_waypoint.y);

        // 3) Steering decision: compute angle difference
        const float desired_heading_radians = std::atan2(vector_to_current_waypoint.y, vector_to_current_waypoint.x);
        const float current_heading_radians = this->current_heading_angle_.asRadians();
        const float angle_difference_radians = std::remainder(desired_heading_radians - current_heading_radians, 2.0f * std::numbers::pi_v<float>);

        this->is_steering_left = (angle_difference_radians < -steering_angle_threshold_radians);
        this->is_steering_right = (angle_difference_radians > steering_angle_threshold_radians);

        // 4) Compute stopping distance needed at current speed
        const float current_speed_magnitude = std::hypot(this->current_velocity_vector_.x, this->current_velocity_vector_.y);
        const float required_stopping_distance = (current_speed_magnitude * current_speed_magnitude) / (2.0f * this->config_.brake_deceleration_pixels_per_second_squared);

        // 5) Decide whether to brake for current or next corner
        const bool should_brake_for_current_corner = (current_waypoint.type == TrackWaypoint::Type::Corner) && (distance_to_current_waypoint <= required_stopping_distance + corner_braking_extra_margin);

        const sf::Vector2f vector_to_next_waypoint = next_waypoint.position - current_position;
        const float distance_to_next_waypoint = std::hypot(vector_to_next_waypoint.x, vector_to_next_waypoint.y);
        const bool should_prebrake_for_next_corner = (next_waypoint.type == TrackWaypoint::Type::Corner) && (distance_to_next_waypoint <= required_stopping_distance + next_corner_prebraking_extra_margin);

        if (should_brake_for_current_corner || should_prebrake_for_next_corner) {
            this->is_braking = true;
            this->is_accelerating = false;
        }
        else {
            this->is_accelerating = true;
            this->is_braking = false;
        }

        // 6) Advance waypoint index when close enough
        if (distance_to_current_waypoint < waypoint_reach_distance) {
            std::size_t advance_index = current_index + 1;
            if (advance_index >= waypoints.size()) {
                advance_index = 1;
            }
            this->current_target_waypoint_index_ = advance_index;
        }

        // 7) Perform shared physics update
        this->handle_physics(dt);
    }

  private:
    /// Index of the current target waypoint (start at 1 to skip spawn point)
    std::size_t current_target_waypoint_index_ = 1;
};

}  // namespace core::game
