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
#include <random>     // for std::mt19937, std::uniform_real_distribution
#include <vector>     // for std::vector

#include <SFML/Graphics.hpp>

#ifndef NDEBUG  // Debug, remove later
#include <imgui.h>
#endif

namespace core::game {

/**
 * @brief Struct that represents the configurable parameters of the track. Invalid values will be clamped to reasonable defaults.
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
#ifndef NDEBUG  // Debug, remove later
    std::size_t vertical_count = 7;
#else
    std::size_t vertical_count = 12;
#endif

    /**
     * @brief Size of each tile in pixels (e.g., "256").
     *
     * @note This does not depend on the size of the texture; it will be scaled to this size.
     *
     * @details The default texture size is 128x128px, so we are scaling it up 6 times.
     */
    std::size_t size_px = 768;

    /**
     * @brief Probability in the range [0.0, 1.0] that a detour bubble will be generated on each vertical edge segment.
     *
     * @note A value of 0.0 disables detours entirely; 1.0 maximizes detour frequency.
     */
#ifndef NDEBUG
    float detour_probability = 0.0f;
#else
    float detour_probability = 0.4f;
#endif

    // /**
    //  * @brief Default three-way comparison operator.
    //  *
    //  * This automatically generates "operator==", "operator<", "operator<=", "operator>", and "operator>=".
    //  */
    // [[nodiscard]] auto operator<=>(const TrackConfig &) const = default;

    // Comparing floats might be a bad idea,
};

/**
 * @brief Struct that represents a single waypoint on the track, including its position and driving type behavior.
 */
struct TrackWaypoint final {
    /**
     * @brief Enum that represents the driving type of the waypoint.
     */
    enum class DrivingType {
        /**
         * @brief Straight-line waypoint; vehicles can maintain full speed.
         */
        Straight,

        /**
         * @brief Corner waypoint; vehicles should slow down, preferably before the corner.
         */
        Corner
    };

    /**
     * @brief World-space coordinates of the waypoint center (e.g., "{100.f, 200.f}").
     */
    sf::Vector2f position;

    /**
     * @brief Driving type of the waypoint (e.g., "Straight" or "Corner").
     */
    DrivingType type;
};

/**
 * @brief Class that manages procedural generation, validation, and rendering of a race track.
 *
 * On construction or configuration change, this class builds the complete track layout from provided tile textures and configuration parameters, which can be drawn to a render target.
 */
class Track final {
  public:
    /**
     * @brief Parameter struct for the track textures. Holds references to the textures used to build the track.
     *
     * The caller is responsible for ensuring that these textures remain valid for the lifetime of the "Track" instance.
     * It is assumed that all textures are square and of the same size (e.g., 256x256) for uniform scaling.
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
     * On construction, the track is automatically built using the provided configuration. The track will be ready for use immediately after construction.
     *
     * @param tiles Tiles struct containing the textures. It is assumed that all textures are square (e.g., 256x256) for uniform scaling. The caller is responsible for ensuring that these textures remain valid for the lifetime of the Track.
     * @param rng Instance of a random number generator (e.g., std::mt19937) used for generating random detours.
     * @param config Configuration struct containing the track configuration (invalid values will be clamped) (default: "TrackConfig()").
     */
    explicit Track(const Textures tiles,  //  Copy to prevent segfault
                   std::mt19937 &rng,
                   const TrackConfig &config = TrackConfig());  // Use default config

    /**
     * @brief Get the current validated track configuration.
     *
     * @return Const reference to the current configuration.
     */
    [[nodiscard]] const TrackConfig &get_config() const;

    /**
     * @brief Set the configuration (invalid values will be clamped), then build the track.
     *
     * @param config New configuration for the track; invalid values are clamped during validation.
     */
    void set_config(const TrackConfig &config);

    /**
     * @brief Reset the track to use the default configuration.
     *
     * This rebuilds the track using the default "TrackConfig", effectively restoring it to its initial state.
     */
    void reset();

    /**
     * @brief Check whether a given world-space point lies within any track tile boundary.
     *
     * This is a simple check that treats every tile as a rectangle, regardless of its actual shape. So while it's technically possible to go outside the curves, the collision detection is simple and fast.
     *
     * @param world_position Coordinates in world space to test.
     *
     * @return True if the point is inside any tile collision bounds; false otherwise.
     */
    [[nodiscard]] bool is_on_track(const sf::Vector2f &world_position) const;

    /**
     * @brief Get the ordered sequence of waypoints.
     *
     * This is used for AI navigation around the track by following the waypoints sequentially.
     *
     * @return Const reference to the vector of TrackWaypoint instances defining the racing line.
     */
    [[nodiscard]] const std::vector<TrackWaypoint> &get_waypoints() const;

    /**
     * @brief Get the world-space position of the finish line spawn point.
     *
     * This is the position of the finish line tile, used as a spawn point for cars.
     *
     * @return Coordinates of the finish-line tile center.
     */
    [[nodiscard]] const sf::Vector2f &get_finish_position() const;

    // TODO: Make it so that the first waypoint (index=0) is always the finish line, instead of requiting the "get_finish_index()" method

    /**
     * @brief Get the index of the finish-line waypoint in the ordered waypoint sequence (vector).
     *
     * @return Zero-based index into the waypoint vector corresponding to the finish point.
     *
     * @note This ensures that the AI cars don't immediately U-turn to the waypoint before the finish line.
     *
     * @details TODO: Find a way to get rid of this and set the "waypoints_[0]" to the finish point.
     */
    [[nodiscard]] std::size_t get_finish_index() const;

    /**
     * @brief Draw all track tile sprites onto the provided render target.
     *
     * @param target Target window where the track will be drawn.
     *
     * @note Call this once per frame.
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
     * @brief Return a copy of the configuration with invalid values clamped to safe values.
     *
     * @param config Configuration for the track, possibly containing invalid values.
     *
     * @return Copy of the configuration with ubvakud values clamped to safe values (e.g., "horizontal_count" and "vertical_count" must be at least "3" or higher).
     *
     * @note Always use this during construction or configuration changes to prevent catastrophic errors.
     */
    [[nodiscard]] TrackConfig validate_config(const TrackConfig &config) const;

    /**
     * @brief Build the track layout using on current configuration and textures.
     *
     * This fills in the sprite array, collision bounds, waypoint sequence, and finish-line data.
     *
     * Random detour bubbles are inserted on vertical edges according to detour probability, whereas the horizontal edges are always straight.
     *
     * @note This is marked as private, because we only want to build the track on construction and explicit config changes.
     */
    void build();

    /**
     * @brief Tile textures used for building the track.
     *
     * @details A copy is required to prevent segfault.
     */
    const Textures tiles_;

    /**
     * @brief Random number generator used for making the track layout more interesting/unpredictable.
     */
    std::mt19937 &rng_;

    /**
     * @brief Current validated track configuration.
     */
    TrackConfig config_;

    /**
     * @brief Collection of sprite objects for each tile in the track layout.
     *
     * @note This should be drawn every frame to display the track on the screen.
     */
    std::vector<sf::Sprite> sprites_;

    /**
     * @brief Axis-aligned bounding rectangles used for collision detection against each sprite.
     *
     * @note This ensures that we don't need to call "getGlobalBounds()" on each sprite every time we want to check for collisions.
     */
    std::vector<sf::FloatRect> collision_bounds_;

    /**
     * @brief Ordered sequence of waypoints defining the AI navigation path around the track.
     */
    std::vector<TrackWaypoint> waypoints_;

    /**
     * @brief Center position of the finish-line sprite, used as a spawn point for vehicles.
     */
    sf::Vector2f finish_point_;

    /**
     * @brief Index of the finish-line waypoint within the waypoint vector.
     *
     * @note This is used for AI navigation - since the first waypoint is not at the finish point (it's at least 1 tile later), we need to know where the finish point is to start the car there, to prevent it from doing an U-turn to the waypoint before the finish point.
     *
     * @details TODO: Find a way to get rid of this and set the "waypoints_[0]" to the finish point.
     */
    std::size_t finish_index_;
};

/**
 * @brief Struct that represents the configurable parameters of the car.
 */
struct CarConfig final {
    /**
     * @brief The rate at  expressed in pixels per second squared.
     *
     * Higher values make the car accelerate more quickly when the gas pedal is pressed.
     */
    float throttle_acceleration_rate_pixels_per_second_squared = 700.0f;

    /**
     * @brief The rate at which applying the foot brake decreases the car's velocity, expressed in pixels per second squared.
     *
     * Larger values cause the car to slow down more aggressively under normal braking.
     */
    float brake_deceleration_rate_pixels_per_second_squared = 950.0f;

    /**
     * @brief The rate at which applying the hand brake (emergency brake) decelerates the car, in pixels per second squared.
     *
     * This is significantly stronger than the regular brake, enabling rapid stops or drifts.
     */
    float handbrake_deceleration_rate_pixels_per_second_squared = 2200.0f;

    /**
     * @brief The passive deceleration applied by engine drag when no input is given, in pixels per second squared.
     *
     * Lower values allow the car to coast longer before coming to a stop.
     */
    float engine_braking_rate_pixels_per_second_squared = 80.0f;

    /**
     * @brief The maximum forward speed the car may attain, in pixels per second.
     *
     * Velocities above this threshold are clamped to prevent unrealistic top speeds.
     */
    float maximum_movement_pixels_per_second = 2500.0f;

    /**
     * @brief The angular turn rate applied to the car's orientation when steering input is held, in degrees per second.
     *
     * Higher values permit sharper turns at a given speed.
     */
    float steering_turn_rate_degrees_per_second = 520.0f;

    /**
     * @brief The rate at which the steering wheel returns to center when no steering input is active, in degrees per second.
     *
     * Higher values cause the car to self-straighten more rapidly.
     */
    float steering_autocenter_rate_degrees_per_second = 580.0f;

    /**
     * @brief The maximum allowed steering wheel angle, in degrees.
     *
     * This determines the furthest angle the wheels (and thus the car) can turn from its forward axis.
     */
    float maximum_steering_angle_degrees = 180.0f;

    /**
     * @brief Multiplier for steering effectiveness at zero speed (full responsiveness).
     *
     * A value of 1.0 gives maximum steering authority when stationary.
     *
     * @details You can't technically move the car at zero speed, but that's the easiest way to explain it.
     */
    float steering_sensitivity_at_zero_speed = 1.0f;

    /**
     * @brief Multiplier for steering effectiveness at maximum speed (reduced responsiveness).
     *
     * Values below 1.0 simulate less agile handling at high velocity.
     */
    float steering_sensitivity_at_maximum_speed = 0.8f;

    /**
     * @brief Lateral slip damping coefficient per second.
     *
     * Higher values reduce sideways sliding more aggressively, promoting stable handling in turns.
     */
    float lateral_slip_damping_coefficient_per_second = 12.0f;

    /**
     * @brief Fraction of velocity retained after a collision bounce.
     *
     * 0.0 means a full stop on impact; 1.0 means a perfectly elastic bounce.
     */
    float collision_velocity_retention_ratio = 0.5f;

    /**
     * @brief Minimum speed required (in pixels per second) for a bounce to occur.
     *
     * Below this threshold, collisions will simply halt the car to avoid jitter.
     */
    float collision_minimum_bounce_speed_pixels_per_second = 50.0f;

    /**
     * @brief Minimum random angle offset (in degrees) applied to the car's rebound direction on collision at low speeds.
     *
     * This creates subtle direction changes at low speeds to prevent jitter while maintaining predictable handling.
     */
    float collision_minimum_random_bounce_angle_degrees = 1.0f;

    /**
     * @brief Maximum random angle offset (in degrees) applied to the car's rebound direction on collision at high speeds.
     *
     * Higher values create more unpredictable bounces at high speeds, which helps prevent getting stuck in walls.
     */
    float collision_maximum_random_bounce_angle_degrees = 35.0f;
};

/**
 * @brief Base class for all cars in the game world, designed to be inherited by custom car classes.
 *
 * Provides core physics simulation, rendering, and collision handling.
 *
 * On construction, the car is initialized with a texture, position, alongside other parameters.
 */
class BaseCar {
  public:
    /**
     * @brief Enum that represents the current gear of the car.
     *
     * The simplest possible gearbox with three gears: Forward, Neutral, and Backward.
     */
    enum class Gear {
        /**
         * @brief Normal: Go forward when holding the gas pedal and brake when holding the brake pedal.
         */
        Forward,

        /**
         * @brief Flip: Go backward when pressing the brake pedal and brake when pressing the gas pedal.
         *
         * @note This doesn't really make sense IRL, but it's the easiest to implement and it works that way in most arcade racing games, such as this one.
         */
        Backward,
    };

    /**
     * @brief Construct a new BaseCar object.
     *
     * @param texture Reference to the SFML texture used for the car sprite. This is expected to be around 71x131 pixels.
     * @param rng Reference to a random number generator used for random decision making (e.g., collision bounces).
     * @param track Reference to the race track object for boundary, spawnpoint and waypoint information.
     * @param config Configuration parameters controlling acceleration, braking, steering, and collision behavior.
     *
     * @note This uses the internal track reference to place the car at the track's spawn point.
     */
    explicit BaseCar(const sf::Texture &texture,
                     std::mt19937 &rng,
                     const Track &track,
                     const CarConfig &config = CarConfig())  // Use default config
        : sprite_(texture),
          track_(track),
          config_(config),
          // Private
          rng_(rng),
          last_position_(this->track_.get_finish_position()),  // Get spawn point from the track
          velocity_(0.0f, 0.0f),
          gear_(Gear::Forward),
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

    /**
     * @brief Destroy the BaseCar object.
     *
     * Virtual destructor to ensure proper cleanup in derived classes.
     */
    virtual ~BaseCar() = default;

    /**
     * @brief Reset the car's position, rotation, velocity, and steering to initial state.
     *
     * This is useful when rebuilding the track or resetting the game.
     *
     * @note This uses the internal track reference to place the car at the track's spawn point.
     */
    virtual void reset()
    {
        const sf::Vector2f spawn_point = this->track_.get_finish_position();
        this->sprite_.setPosition(spawn_point);
        this->sprite_.setRotation(sf::degrees(0.0f));
        this->last_position_ = spawn_point;
        this->velocity_ = {0.0f, 0.0f};
        this->steering_wheel_angle_ = 0.0f;
    }

    /**
     * @brief Get the current position of the car.
     *
     * @return Vector containing the car's position in world coordinates (pixels).
     */
    [[nodiscard]] sf::Vector2f get_position() const
    {
        return this->sprite_.getPosition();
    }

    /**
     * @brief Get the current velocity of the car.
     *
     * @return Vector containing the car's velocity in pixels per second.
     *
     * @note If you want to display speed in a car's speedometer sense, use "get_speed()" instead. This is the raw velocity vector for physics calculations.
     */
    [[nodiscard]] sf::Vector2f get_velocity() const
    {
        return this->velocity_;
    }

    /**
     * @brief Get the current speed of the car based on the current velocity vector.
     *
     * @return Speed magnitude in pixels per second (e.g., "100.f").
     *
     * @note This calculates the scalar speed from the velocity vector before returning it, making it a bit more expensive than simply returning the velocity vector via "get_velocity()".
     */
    [[nodiscard]] float get_speed() const
    {
        return std::hypot(this->velocity_.x, this->velocity_.y);
    }

    /**
     * @brief Get the current gear of the car.
     *
     * @return Current gear of the car (e.g., "Forward", "Neutral", "Backward").
     */
    [[nodiscard]] Gear get_gear() const
    {
        return this->gear_;
    }

    /**
     * @brief Get current steering wheel angle.
     *
     * This is the angle of the steering wheel in degrees, which is used to determine the car's turning radius.
     *
     * @return Steering wheel angle in degrees (e.g., "-180.f", "30.f", "180.f").
     */
    [[nodiscard]] float get_steering_angle() const
    {
        return this->steering_wheel_angle_;
    }

    /**
     * @brief Apply gas to the car immediately.
     *
     * @note Call this every frame to accelerate the car. Do not call it to stop gas.
     */
    virtual void gas()
    {
        this->is_accelerating_ = true;
    }

    /**
     * @brief Apply left foot brake to the car immediately.
     *
     * @note Call this every frame to decelerate the car. Do not call it to stop braking.
     */
    virtual void brake()
    {
        this->is_braking_ = true;
    }

    /**
     * @brief Apply left steering to the car with steering wheel emulation (turn the steering wheel left over time until it reaches the maximum angle, at which point it will stay at that angle).
     *
     * @note Call this every frame to steer left. Do not call it to stop steering left and return to center over time.
     */
    void steer_left()
    {
        this->is_steering_left_ = true;
    }

    /**
     * @brief Apply right steering to the car with steering wheel emulation (turn the steering wheel right over time until it reaches the maximum angle, at which point it will stay at that angle).
     *
     * @note Call this every frame to steer right. Do not call it to stop steering right and return to center over time.
     */
    void steer_right()
    {
        this->is_steering_right_ = true;
    }

    /**
     * @brief Apply handbrake (emergency brake) to the car immediately.
     *
     * @note Call this every frame to decelerate the car. Do not call it to stop handbraking.
     */
    void handbrake()
    {
        this->is_handbraking_ = true;
    }

    /**
     * @brief Update the car's physics state over a time interval.
     *
     * @param dt Time passed since the previous frame, in seconds.
     *
     * @note Call this method once per frame before calling "draw()". It will call the internal "apply_physics_step()" function to apply all the physics calculations, such as acceleration, slip, collision, etc.
     */
    virtual void update(const float dt)
    {
        this->apply_physics_step(dt);
    }

    /**
     * @brief Draw the car on the provided render target.
     *
     * @param target Target window where the car will be drawn.
     *
     * @note Call this once per frame, after "update()".
     */
    virtual void draw(sf::RenderTarget &target) const  // TODO: Remove this later virtual after AI is done and doesn't need debug shapes
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
     * @brief Set the car's gear.
     *
     * @param gear Gear to set (e.g., "Forward", "Neutral", "Backward").
     *
     * @note This is set explicitly by derived classes that implement their own gear logic. For the player, automatic gear shifting when holding the brake for X amount of time makes sense. For AI, it might be better to always use "Forward" gear, unless it is explicitly trying to reverse out of a wall after crashing.
     */
    void set_gear(const Gear gear)
    {
        this->gear_ = gear;
    }

    /**
     * @brief Car sprite object for rendering. Also used for motion and rotation.
     */
    sf::Sprite sprite_;

    /**
     * @brief Reference to the race track for collision detection and waypoint data.
     */
    const Track &track_;

    /**
     * @brief Configuration of the car.
     */
    const CarConfig config_;

  private:
    /**
     * @brief Apply physics step to the car - combines all forces, slip, and collisions.
     *
     * This processeds the member input flags, then runs the entire physics pipeline.
     *
     * @param dt Time passed since the previous frame, in seconds.
     *
     * @note Always call this as the final step in the update loop from "update()", before drawing the car via "draw()".
     */
    void apply_physics_step(const float dt)
    {
        constexpr float stopped_speed_threshold = 0.01f;             // Car counts as stopped below this speed
        constexpr float steering_autocenter_epsilon_degrees = 0.1f;  // Steering snaps to zero inside this range
        constexpr float minimum_speed_for_rotation = 1.0f;           // Absolute forward speed required for sprite rotation

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
        if (this->is_braking_ && current_speed > stopped_speed_threshold) {
            const float brake_reduction = std::min(this->config_.brake_deceleration_rate_pixels_per_second_squared * dt, current_speed);
            const sf::Vector2f velocity_unit_vector = this->velocity_ / current_speed;
            this->velocity_ -= velocity_unit_vector * brake_reduction;
            current_speed -= brake_reduction;
        }

        // Apply handbrake deceleration
        if (this->is_handbraking_ && current_speed > stopped_speed_threshold) {
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
        if (!this->is_accelerating_ && !this->is_braking_ && !this->is_handbraking_ && current_speed > stopped_speed_threshold) {
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
            if (std::abs(this->steering_wheel_angle_) > steering_autocenter_epsilon_degrees && current_speed > 0.0f) {
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
        if (std::abs(signed_forward_speed) > minimum_speed_for_rotation) {
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
                const float speed_ratio = std::clamp((current_speed - this->config_.collision_minimum_bounce_speed_pixels_per_second) / (this->config_.maximum_movement_pixels_per_second - this->config_.collision_minimum_bounce_speed_pixels_per_second), 0.0f, 1.0f);

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

    /**
     * @brief Random number generator.
     */
    std::mt19937 &rng_;

    /**
     * @brief Last valid position of the car sprite in world coordinates (pixels).
     */
    sf::Vector2f last_position_;

    /**
     * @brief Current velocity of the car in pixels per second.
     */
    sf::Vector2f velocity_;

    /**
     * @brief Current gear of the car.
     *
     * This is used to determine the car's behavior when gas and brake are pressed.
     */
    Gear gear_;

    /**
     * @brief Set to true via "gas()" to accelerate the car.
     */
    bool is_accelerating_;

    /**
     * @brief Set to true via "brake()" to apply the foot brake.
     */
    bool is_braking_;

    /**
     * @brief Set to true via "steer_left()" to turn the steering wheel left.
     */
    bool is_steering_left_;

    /**
     * @brief Set to true via "steer_right()" to turn the steering wheel right.
     */
    bool is_steering_right_;

    /**
     * @brief Set to true via "handbrake()" to apply the handbrake (emergency brake).
     */
    bool is_handbraking_;

    /**
     * @brief Current steering wheel angle in degrees. This emulates a steering wheel via "steer_left()" and "steer_right()". If they are not called, the steering wheel will return to center over time.
     *
     * Positive values turn the car right, negative values turn it left.
     */
    float steering_wheel_angle_;
};

/**
 * @brief Player-controlled car class.
 *
 * Inherits core physics and rendering from BaseCar.
 *
 * On construction, it runs the base class constructor.
 */
class PlayerCar final : public BaseCar {
  public:
    // Inherit the constructor from BaseCar
    using BaseCar::BaseCar;

    // Ensure compilation fails if BaseCar's destructor ever stops being virtual
    ~PlayerCar() override = default;

    // TODO: Override this to implement switching to reverse when holding for X amount of time
    // We probably need to store "dt" from "update()" in a member variable to do this?
    void gas() override
    {
        BaseCar::gas();

        // If below minimum speed and holding the gas for long enough, switch to forward gear
    }

    void brake() override
    {
        BaseCar::brake();

        // If below minimum speed and holding the brake for long enough, switch to reverse gear
    }

  private:
    // Minimum speed at which we allow the car to switch gears
    static constexpr float minimum_speed_for_gear_switch = 5.f;

    // Time to wait in seconds before switching to reverse gear, must be holding the gas or brake pedal for that
    static constexpr float time_to_switch_to_reverse = 0.5f;
};
/**
 * @brief AI-controlled car class.
 *
 * Inherits core physics and rendering from BaseCar, implements waypoint‑following logic to drive around the track.
 *
 * On construction, it runs the base class constructor.
 */
class AICar final : public BaseCar {
  public:
    // Inherit from BaseCar but also set waypoint index
    explicit AICar(const sf::Texture &texture,
                   std::mt19937 &rng,
                   const Track &track,
                   const CarConfig &config = CarConfig())  // Use default config
        : BaseCar(texture, rng, track, config),
          current_waypoint_index_number_(this->track_.get_finish_index() + 1)  // Start at the finish waypoint index + 1
    {
        // Setup debug shape
        this->debug_shape_.setFillColor({255, 0, 0, 64});  // 25% opacity red
        this->debug_shape_.setOrigin(this->debug_shape_.getLocalBounds().getCenter());
    }

    // Ensure compilation fails if BaseCar's destructor ever stops being virtual
    ~AICar() override = default;

    /**
     * @brief Reset the car's position, rotation, velocity, and steering to initial state.
     *
     * This is useful when rebuilding the track or resetting the game.
     *
     * @note This uses the internal track reference to get the current spawn point. This also resets the current AI waypoint index to 1, skipping the spawn point (index 0) to prevent issues.
     */
    void reset() override
    {
        // Call base class reset to handle sprite and physics
        BaseCar::reset();

        // Must also reset the current waypoint index, but ignore the spawn point (so we add 1)
        this->current_waypoint_index_number_ = this->track_.get_finish_index() + 1;
    }

    void draw(sf::RenderTarget &target) const override
    {
        // Draw the debug shape
        target.draw(this->debug_shape_);

        // Draw the base class sprite
        BaseCar::draw(target);
    }

    /**
     * @brief Update the AI car's steering and throttle/brake decisions based on waypoints over a time interval, then update the car's physics state.
     *
     * @param dt Time passed since the previous frame, in seconds.
     *
     * @note Call this method once per frame before calling "draw()". It will perform AI calculations to determine the best steering and throttle/brake inputs based on the current track and waypoints, then call the base class "update()" method to apply the physics calculations.
     */
    void update(const float dt) override
    {
        // Get basic info
        const auto &waypoints = this->track_.get_waypoints();
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

        // Use more aggressive steering when approaching or at corners
        float steering_threshold;
        if (at_corner || approaching_corner) {
            steering_threshold = this->corner_steering_threshold_;
        }
        else {
            steering_threshold = this->straight_steering_threshold_;
        }

        // Early corner turning: if approaching corner and close enough, use corner threshold
        if (approaching_corner && distance_to_current_waypoint < tile_size * this->early_corner_turn_distance_) {
            steering_threshold = this->corner_steering_threshold_;
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

        // Speed management - fixed logic to prevent getting stuck
        const float target_speed =
            (current_waypoint.type == TrackWaypoint::DrivingType::Corner || next_waypoint.type == TrackWaypoint::DrivingType::Corner)
                ? tile_size * this->corner_speed_factor_
                : tile_size * this->straight_speed_factor_;

        const float brake_distance = tile_size * this->brake_distance_factor_;

        // More intelligent braking logic
        const bool approaching_corner_too_fast = (next_waypoint.type == TrackWaypoint::DrivingType::Corner) &&
                                                 (distance_to_current_waypoint < brake_distance) &&
                                                 (current_speed > target_speed * 1.5f);

        const bool speed_too_high = current_speed > target_speed * 2.0f;  // Only brake if way too fast

        const bool should_brake = collision_detected || speed_too_high || approaching_corner_too_fast;

        if (should_brake) {
            this->brake();
        }
        else if (current_speed < target_speed * 0.8f) {  // Start accelerating sooner
            this->gas();
        }

        // Advance waypoint
        if (distance_to_current_waypoint < waypoint_reach_distance) {
            this->current_waypoint_index_number_ = next_index;
        }

        // Update debug shape
        this->debug_shape_.setRadius(waypoint_reach_distance);
        this->debug_shape_.setOrigin(this->debug_shape_.getLocalBounds().getCenter());  // Recalculate origin after radius change
        this->debug_shape_.setPosition(current_waypoint.position);

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

                ImGui::SliderFloat("Waypoint Reach", &this->waypoint_reach_factor_, 0.1f, 1.0f, "%.2fx");
                ImGui::TextWrapped("How close the car must get to a waypoint before targeting the next one. Lower = more precise but may miss waypoints.");

                ImGui::SliderFloat("Collision Distance", &this->collision_distance_, 0.1f, 1.0f, "%.2fx");
                ImGui::TextWrapped("How far ahead the car looks for track boundaries to avoid crashes. Higher = earlier collision avoidance.");

                // Steering settings
                ImGui::SeparatorText("Steering Parameters");

                ImGui::SliderFloat("Straight Threshold", &this->straight_steering_threshold_, 0.05f, 0.5f, "%.3f rad");
                ImGui::TextWrapped("How much the car heading must differ from target direction before steering on straights. Higher = less wiggling but slower corrections.");

                ImGui::SliderFloat("Corner Threshold", &this->corner_steering_threshold_, 0.001f, 0.1f, "%.3f rad");
                ImGui::TextWrapped("How much the car heading must differ from target direction before steering in corners. Lower = more responsive turning.");

                ImGui::SliderFloat("Min Straight Steer", &this->minimum_straight_steering_difference_, 0.01f, 0.2f, "%.3f rad");
                ImGui::TextWrapped("Minimum heading difference required to steer on straights. Prevents tiny steering corrections that cause wiggling.");

                ImGui::SliderFloat("Early Corner Turn", &this->early_corner_turn_distance_, 0.5f, 2.0f, "%.2fx");
                ImGui::TextWrapped("How far before a corner the car starts using corner steering settings. Higher = starts turning sooner for smoother cornering.");

                // Speed settings
                ImGui::SeparatorText("Speed Parameters");

                ImGui::SliderFloat("Corner Speed", &this->corner_speed_factor_, 0.5f, 2.0f, "%.2fx");
                ImGui::TextWrapped("Target speed multiplier for corners. Lower = slower and safer cornering, higher = faster but riskier.");

                ImGui::SliderFloat("Straight Speed", &this->straight_speed_factor_, 1.0f, 3.0f, "%.2fx");
                ImGui::TextWrapped("Target speed multiplier for straight sections. Higher = faster top speed on straights.");

                ImGui::SliderFloat("Brake Distance", &this->brake_distance_factor_, 0.2f, 2.0f, "%.2fx");
                ImGui::TextWrapped("How far before corners the car starts braking. Higher = earlier braking for safer cornering.");

                ImGui::PopItemWidth();

                ImGui::Spacing();

                // Reset button
                if (ImGui::Button("Reset to Defaults", ImVec2(-1.0f, 0.0f))) {
                    this->waypoint_reach_factor_ = 0.5f;
                    this->collision_distance_ = 0.5f;
                    this->straight_steering_threshold_ = 0.36f;
                    this->corner_steering_threshold_ = 0.02f;
                    this->minimum_straight_steering_difference_ = 0.05f;
                    this->early_corner_turn_distance_ = 1.0f;
                    this->corner_speed_factor_ = 1.2f;
                    this->straight_speed_factor_ = 2.0f;
                    this->brake_distance_factor_ = 1.0f;
                }

                ImGui::Unindent();
            }
        }
        ImGui::End();
#endif

        // Apply physics
        BaseCar::update(dt);
    }

  private:
    // Runtime-configurable AI parameters (exposed in debug window)
    float waypoint_reach_factor_ = 0.5f;                  // Waypoint reach distance as fraction of tile size
    float collision_distance_ = 0.5f;                     // Collision check distance as fraction of tile size
    float straight_steering_threshold_ = 0.2f;            // Steering threshold on straights (higher = less wiggling)
    float corner_steering_threshold_ = 0.02f;             // Steering threshold in corners (lower = more responsive)
    float minimum_straight_steering_difference_ = 0.05f;  // Minimum heading difference to steer on straights (reduces wiggling)
    float early_corner_turn_distance_ = 1.0f;             // Distance before corner to start turning early (as fraction of tile size)
    float corner_speed_factor_ = 1.2f;                    // Target speed in corners as fraction of tile size
    float straight_speed_factor_ = 2.0f;                  // Target speed on straights as fraction of tile size
    float brake_distance_factor_ = 1.0f;                  // Brake distance as fraction of tile size

    // Debug shape for visualization
    sf::CircleShape debug_shape_{250.0f};

    /**
     * @brief Index of the current target waypoint.
     */
    std::size_t current_waypoint_index_number_;
};

}  // namespace core::game
