/**
 * @file game.hpp
 *
 * @brief Game world map abstractions.
 */

#pragma once

#include <algorithm>  // for std::clamp, std::min, std::max
#include <cmath>      // for std::hypot, std::remainder, std::atan2, std::sin, std::cos, std::lerp, std::abs
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

    // Disable move semantics
    Track(Track &&) = delete;
    Track &operator=(Track &&) = delete;

    // Disable copy semantics
    Track(const Track &) = delete;
    Track &operator=(const Track &) = delete;

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
    float collision_speed_retention_factor = 0.5f;

    /// Minimum speed for collision bounce in pixels per second, preventing cars from bouncing at low speeds
    float collision_minimal_bounce_speed_pixels_per_second = 50.0f;

    // Maximum random bounce angle in degrees, only applied if above "collision_minimal_bounce_speed_pixels_per_second"
    float collision_random_bounce_max_degrees = 15.0f;
};

class BaseCar {
  public:
    explicit BaseCar(const sf::Texture &texture,
                     const sf::Vector2f &initial_position,
                     std::mt19937 &rng,
                     const Track &track,
                     const CarConfig &config = CarConfig())
        : sprite_(texture),
          rng_(rng),
          track_(track),
          config_(config),
          collision_jitter_dist_(-config_.collision_random_bounce_max_degrees, config_.collision_random_bounce_max_degrees),
          last_position_(initial_position),
          velocity_(0.0f, 0.0f),
          steering_wheel_angle_(0.0f),
          speed_magnitude_pixels_per_second_(0.0f),
          is_accelerating_(false),
          is_braking_(false),
          is_handbraking_(false),
          is_steering_left_(false),
          is_steering_right_(false)
    {
        // Center the sprite origin for correct rotation and positioning
        this->sprite_.setOrigin(this->sprite_.getLocalBounds().getCenter());
        // Set initial position of the car sprite
        this->sprite_.setPosition(initial_position);
    }

    // TODO: Check if this is needed if we have other virtual functions
    virtual ~BaseCar() = default;

    /**
     * @brief Reset the car to a given position with zero velocity and zero steering.
     */
    virtual void reset(const sf::Vector2f &reset_position)
    {
        this->sprite_.setPosition(reset_position);
        this->sprite_.setRotation(sf::degrees(0.0f));
        this->last_position_ = reset_position;
        this->velocity_ = {0.0f, 0.0f};
        this->steering_wheel_angle_ = 0.0f;
    }

    /**
     * @brief Get current position of the car sprite in pixels.
     */
    [[nodiscard]] sf::Vector2f get_position() const
    {
        return this->sprite_.getPosition();
    }

    /**
     * @brief Get current velocity vector in pixels per second.
     */
    [[nodiscard]] sf::Vector2f get_velocity() const
    {
        return this->velocity_;
    }

    /**
     * @brief Update physics for the car based on elapsed time.
     */
    virtual void update(const float dt)
    {
        this->handle_physics(dt);
    }

    /**
     * @brief Draw the car sprite onto the given render target.
     */
    void draw(sf::RenderTarget &target) const
    {
        target.draw(this->sprite_);
    }

    // Disable move semantics
    BaseCar(BaseCar &&) = delete;
    BaseCar &operator=(BaseCar &&) = delete;

    // Disable copy semantics
    BaseCar(const BaseCar &) = delete;
    BaseCar &operator=(const BaseCar &) = delete;

  protected:
    /**
     * @brief Compute the current speed magnitude of the car.
     * @return Speed magnitude in pixels per second.
     */
    [[nodiscard]] float compute_speed_pixels_per_second() const
    {
        return std::hypot(this->velocity_.x, this->velocity_.y);
    }

    /**
     * @brief Apply forward acceleration based on current heading.
     */
    void accelerate(const float dt)
    {
        // Convert heading angle to radians for math functions
        const float heading_angle_radians = this->sprite_.getRotation().asRadians();

        // Increase velocity components along heading direction
        this->velocity_.x += std::cos(heading_angle_radians) * this->config_.forward_acceleration_pixels_per_second_squared * dt;
        this->velocity_.y += std::sin(heading_angle_radians) * this->config_.forward_acceleration_pixels_per_second_squared * dt;
    }

    /**
     * @brief Apply braking deceleration until stopping.
     */
    void brake(const float dt)
    {
        // Compute current speed magnitude
        const float speed_magnitude_pixels_per_second = std::hypot(this->velocity_.x, this->velocity_.y);

        // Only brake if moving above a tiny threshold
        if (speed_magnitude_pixels_per_second > 0.01f) {
            // Unit direction of current velocity
            const float unit_velocity_x = this->velocity_.x / speed_magnitude_pixels_per_second;
            const float unit_velocity_y = this->velocity_.y / speed_magnitude_pixels_per_second;
            // Compute speed reduction this frame
            const float maximum_reduction_pixels_per_second = this->config_.brake_deceleration_pixels_per_second_squared * dt;
            const float actual_reduction_pixels_per_second = std::min(maximum_reduction_pixels_per_second, speed_magnitude_pixels_per_second);
            // Subtract reduction along velocity direction
            this->velocity_.x -= unit_velocity_x * actual_reduction_pixels_per_second;
            this->velocity_.y -= unit_velocity_y * actual_reduction_pixels_per_second;
        }
    }

    /**
     * @brief Apply handbrake deceleration.
     */
    void handbrake(const float dt)
    {
        const float speed_magnitude_pixels_per_second = std::hypot(this->velocity_.x, this->velocity_.y);
        if (speed_magnitude_pixels_per_second > 0.01f) {
            const float unit_velocity_x = this->velocity_.x / speed_magnitude_pixels_per_second;
            const float unit_velocity_y = this->velocity_.y / speed_magnitude_pixels_per_second;
            // Get new speed after applying handbrake
            const float reduced_speed_pixels_per_second = speed_magnitude_pixels_per_second - this->config_.handbrake_deceleration_pixels_per_second_squared * dt;
            if (reduced_speed_pixels_per_second < 0.1f) {
                // If almost stopped, zero velocity
                this->velocity_ = {0.0f, 0.0f};
            }
            else {
                // Otherwise reduce speed but keep direction
                this->velocity_.x = unit_velocity_x * reduced_speed_pixels_per_second;
                this->velocity_.y = unit_velocity_y * reduced_speed_pixels_per_second;
            }
        }
    }

    /**
     * @brief Turn steering wheel left by decreasing angle.
     */
    void steer_left(const float dt)
    {
        this->steering_wheel_angle_ -= this->config_.steering_turn_rate_degrees_per_second * dt;
    }

    /**
     * @brief Turn steering wheel right by increasing angle.
     */
    void steer_right(const float dt)
    {
        this->steering_wheel_angle_ += this->config_.steering_turn_rate_degrees_per_second * dt;
    }

    /**
     * @brief Core physics handling: inputs, engine braking, slip, steering, collision.
     */
    void handle_physics(const float dt)
    {
        // Step 1: Apply input forces
        if (this->is_accelerating_) {
            this->accelerate(dt);
        }
        if (this->is_braking_) {
            this->brake(dt);
        }
        if (this->is_handbraking_) {
            this->handbrake(dt);
        }
        if (this->is_steering_left_) {
            this->steer_left(dt);
        }
        if (this->is_steering_right_) {
            this->steer_right(dt);
        }

        // Step 2: Compute and cache current speed in pixels per second
        this->speed_magnitude_pixels_per_second_ = this->compute_speed_pixels_per_second();
        const float initial_speed_pixels_per_second = this->speed_magnitude_pixels_per_second_;

        // Step 3: Apply engine braking when no input is active
        if (!this->is_accelerating_ && !this->is_braking_ && !this->is_handbraking_ && initial_speed_pixels_per_second > 0.01f) {
            const float engine_brake_amount = this->config_.engine_braking_pixels_per_second_squared * dt;
            const float reduced_speed = (initial_speed_pixels_per_second > engine_brake_amount) ? (initial_speed_pixels_per_second - engine_brake_amount) : 0.0f;
            const float engine_brake_scale = reduced_speed / initial_speed_pixels_per_second;
            this->velocity_.x *= engine_brake_scale;
            this->velocity_.y *= engine_brake_scale;
        }

        // Step 4: Enforce maximum speed limit
        const float speed_after_brake = this->compute_speed_pixels_per_second();
        const float maximum_speed = this->config_.maximum_speed_pixels_per_second;
        if (speed_after_brake > maximum_speed) {
            const float maximum_speed_scale = maximum_speed / speed_after_brake;
            this->velocity_.x *= maximum_speed_scale;
            this->velocity_.y *= maximum_speed_scale;
        }

        // Step 5: Record speed before slip damping
        const float pre_slip_speed_pixels_per_second = this->compute_speed_pixels_per_second();

        // Step 6: Apply side slip damping to reduce lateral velocity
        const float heading_radians = this->sprite_.getRotation().asRadians();
        const sf::Vector2f forward_unit_vector{std::cos(heading_radians), std::sin(heading_radians)};
        const float forward_speed_component = forward_unit_vector.x * this->velocity_.x + forward_unit_vector.y * this->velocity_.y;
        const sf::Vector2f forward_velocity_vector = forward_unit_vector * forward_speed_component;
        const sf::Vector2f lateral_velocity_vector = this->velocity_ - forward_velocity_vector;
        const float slip_damping_ratio = 1.0f - std::clamp(this->config_.side_slip_damping_coefficient_per_second * dt, 0.0f, 1.0f);
        this->velocity_ = forward_velocity_vector + lateral_velocity_vector * slip_damping_ratio;

        // Step 7: Center steering wheel if no steering input is active
        constexpr float steering_angle_epsilon_degrees = 0.1f;
        if (!this->is_steering_left_ && !this->is_steering_right_) {
            if (pre_slip_speed_pixels_per_second > 0.0f && std::abs(this->steering_wheel_angle_) > steering_angle_epsilon_degrees) {
                const float centering_rate = this->config_.steering_centering_rate_degrees_per_second;
                const float centering_interp = std::clamp(centering_rate * dt / std::abs(this->steering_wheel_angle_), 0.0f, 1.0f);
                this->steering_wheel_angle_ = std::lerp(this->steering_wheel_angle_, 0.0f, centering_interp);
            }
            else {
                this->steering_wheel_angle_ = 0.0f;
            }
        }

        // Step 8: Clamp steering wheel angle to allowed range
        this->steering_wheel_angle_ = std::clamp(this->steering_wheel_angle_, -this->config_.maximum_steering_wheel_angle_degrees, this->config_.maximum_steering_wheel_angle_degrees);

        // Step 9: Compute turn factor based on current speed
        const float speed_ratio = std::clamp(pre_slip_speed_pixels_per_second / maximum_speed, 0.0f, 1.0f);
        const float turn_factor_at_current_speed = this->config_.turn_factor_at_zero_speed * (1.0f - speed_ratio) + this->config_.turn_factor_at_full_speed * speed_ratio;

        // Step 10: Apply rotation from steering and move sprite by velocity
        this->sprite_.rotate(sf::degrees(this->steering_wheel_angle_ * turn_factor_at_current_speed * dt));
        this->sprite_.move(this->velocity_ * dt);

        // Step 11: Detect collision and apply bounce with random jitter
        const sf::Vector2f current_position = this->sprite_.getPosition();
        if (!this->track_.is_on_track(current_position)) {
            // Revert to last valid position
            this->sprite_.setPosition(this->last_position_);
            // Reflect and dampen velocity
            this->velocity_ = -this->velocity_ * this->config_.collision_speed_retention_factor;

            // Compute post‐bounce speed
            const float post_bounce_speed = this->compute_speed_pixels_per_second();
            if (post_bounce_speed < this->config_.collision_minimal_bounce_speed_pixels_per_second) {
                this->velocity_ = {0.0f, 0.0f};
            }
            else {
                // Apply a small random yaw jitter to the physics velocity
                const float jitter_degrees = this->collision_jitter_dist_(this->rng_);
                const float jitter_radians = sf::degrees(jitter_degrees).asRadians();
                const float jitter_cosine = std::cos(jitter_radians);
                const float jitter_sine = std::sin(jitter_radians);
                const sf::Vector2f old_velocity = this->velocity_;
                this->velocity_.x = old_velocity.x * jitter_cosine - old_velocity.y * jitter_sine;
                this->velocity_.y = old_velocity.x * jitter_sine + old_velocity.y * jitter_cosine;

                // Also rotate sprite visually by the same jitter
                this->sprite_.rotate(sf::degrees(jitter_degrees));
            }
        }

        // Step 12: Reset input flags and store last valid position
        this->is_accelerating_ = false;
        this->is_braking_ = false;
        this->is_handbraking_ = false;
        this->is_steering_left_ = false;
        this->is_steering_right_ = false;
        this->last_position_ = this->sprite_.getPosition();
    }

    // Protected member variables for subclasses
    sf::Sprite sprite_;                                            // Car sprite / texture
    std::mt19937 &rng_;                                            // RNG for physics/AI variation
    const Track &track_;                                           // Track for collision checks and waypoints
    const CarConfig config_;                                       // Car config
    std::uniform_real_distribution<float> collision_jitter_dist_;  // Random bounce angle distribution
    sf::Vector2f last_position_;                                   // Last valid position of the car sprite in pixels
    sf::Vector2f velocity_;                                        // Current velocity vector in pixels per second
    float steering_wheel_angle_;                                   // Current steering wheel angle in degrees
    float speed_magnitude_pixels_per_second_;                      /// Cached speed magnitude in pixels per second

    // Use these to control the car from subclasses; physics will be applied automatically
    bool is_accelerating_;    // If throttle is pressed
    bool is_braking_;         // If brake is pressed
    bool is_handbraking_;     // If handbrake is pressed
    bool is_steering_left_;   // If steering left
    bool is_steering_right_;  // If steering right
};

/**
 * @brief Player-controlled car: processes direct input for driving.
 */
class PlayerCar final : public BaseCar {
  public:
    using BaseCar::BaseCar;

    /**
     * @brief Set player input flags for acceleration, braking, steering, and handbrake.
     */
    void set_input(const bool gas,
                   const bool brake,
                   const bool left,
                   const bool right,
                   const bool handbrake)
    {
        this->is_accelerating_ = gas;
        this->is_braking_ = brake;
        this->is_steering_left_ = left;
        this->is_steering_right_ = right;
        this->is_handbraking_ = handbrake;
    }
};

/**
 * @brief AI-driven car: follows waypoints and slows for corners.
 */
class AICar final : public BaseCar {
  public:
    using BaseCar::BaseCar;

    /**
     * @brief Reset AI car and set initial waypoint index.
     */
    void reset(const sf::Vector2f &reset_position) override
    {
        // Call base class reset to handle sprite and physics
        BaseCar::reset(reset_position);

        // Must also reset the current waypoint index, but ignore the spawn point (index 0)
        this->current_waypoint_index_number_ = 1;
    }

    /**
     * @brief Update AI car: compute steering and braking based on waypoints.
     */
    void update(const float dt) override
    {
        // Define how close the car must be to a waypoint before we mark it "reached"
        constexpr float waypoint_reach_distance_pixels = 10.0f;

        // Define the minimum heading difference (in radians) needed to trigger a steer command
        constexpr float steering_threshold_radians = 0.02f;

        // 1) Retrieve the list of waypoints from the track
        const auto &waypoints = this->track_.get_waypoints();

        // 2) Remember which waypoint we are currently targeting
        const std::size_t current_index = this->current_waypoint_index_number_;

        // 3) Compute the index of the next waypoint, wrapping back to 1 when we reach the end
        const std::size_t next_index = (current_index + 1 < waypoints.size()) ? (current_index + 1) : 1;

        // 4) Get references to the current and next waypoint objects for easy access
        const TrackWaypoint &current_waypoint = waypoints[current_index];
        const TrackWaypoint &next_waypoint = waypoints[next_index];

        // 5) Read the car’s current position in world pixels
        const sf::Vector2f car_position = this->sprite_.getPosition();

        // 6) Compute vector and distance from car to current waypoint
        const sf::Vector2f vector_to_current_waypoint = current_waypoint.position - car_position;
        const float distance_to_current_waypoint = std::hypot(vector_to_current_waypoint.x, vector_to_current_waypoint.y);

        // 7) Compute the desired heading (angle) to face the current waypoint
        const float desired_heading_radians = std::atan2(vector_to_current_waypoint.y, vector_to_current_waypoint.x);

        // 8) Read the car’s current heading (angle) in radians
        const float current_heading_radians = this->sprite_.getRotation().asRadians();

        // 9) Compute the smallest signed angle difference between desired and current heading
        const float heading_difference_radians = std::remainder(desired_heading_radians - current_heading_radians, 2.0f * std::numbers::pi_v<float>);

        // 10) Decide steering direction based on whether the heading error exceeds threshold
        this->is_steering_left_ = (heading_difference_radians < -steering_threshold_radians);
        this->is_steering_right_ = (heading_difference_radians > steering_threshold_radians);

        // 11) Compute current speed magnitude (pixels per second)
        const float current_speed = std::hypot(this->velocity_.x, this->velocity_.y);

        // 12) Compute stopping distance needed at current speed using standard kinematic formula
        const float required_stopping_distance = (current_speed * current_speed) / (2.0f * this->config_.brake_deceleration_pixels_per_second_squared);

        // 13) Compute vector and distance to the next waypoint for pre‑braking checks
        const sf::Vector2f vector_to_next_waypoint = next_waypoint.position - car_position;
        const float distance_to_next_waypoint = std::hypot(vector_to_next_waypoint.x, vector_to_next_waypoint.y);

        // 14) Determine if we should brake for the current corner
        // True if current waypoint is a corner and we are within stopping distance + small margin
        const bool should_brake_current = (current_waypoint.type == TrackWaypoint::Type::Corner) && (distance_to_current_waypoint <= required_stopping_distance + 5.0f);

        // 15) Determine if we should pre‑brake for the next corner
        // True if next waypoint is a corner and we are within stopping distance + larger margin
        const bool should_prebrake_next = (next_waypoint.type == TrackWaypoint::Type::Corner) && (distance_to_next_waypoint <= required_stopping_distance + 15.0f);

        // 16) Apply brake or throttle based on corner logic
        if (should_brake_current || should_prebrake_next) {
            this->is_braking_ = true;
            this->is_accelerating_ = false;
        }
        else {
            this->is_accelerating_ = true;
            this->is_braking_ = false;
        }

        // 17) If we have reached (or passed) the current waypoint, advance to the next
        if (distance_to_current_waypoint < waypoint_reach_distance_pixels) {
            this->current_waypoint_index_number_ = next_index;
        }

        // 18) Finally, call the shared physics handler to apply movement, rotation, and collisions
        this->handle_physics(dt);
    }

  private:
    // Index of the current target waypoint (start at 1 to skip spawn point)
    std::size_t current_waypoint_index_number_ = 1;
};

}  // namespace core::game
