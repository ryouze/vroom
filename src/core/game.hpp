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
    float detour_probability = 1.0f;
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
    float steering_autocenter_rate_degrees_per_second = 780.0f;

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
     * Higher values reduce sideways sliding more aggressively, promoting stable handling in turns. Less makes the car more prone to drifting.
     */
    float lateral_slip_damping_coefficient_per_second = 3.0f;  // Make it drift

    /**
     * @brief Fraction of velocity retained after a collision bounce.
     *
     * 0.0 means a full stop on impact; 1.0 means a perfectly elastic bounce.
     */
    float collision_velocity_retention_ratio = 0.25f;

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
                     const CarConfig &config = CarConfig());  // Use default config

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
    virtual void reset();

    /**
     * @brief Get the current position of the car.
     *
     * @return Vector containing the car's position in world coordinates (pixels).
     */
    [[nodiscard]] sf::Vector2f get_position() const;

    /**
     * @brief Get the current velocity of the car.
     *
     * @return Vector containing the car's velocity in pixels per second.
     *
     * @note If you want to display speed in a car's speedometer sense, use "get_speed()" instead. This is the raw velocity vector for physics calculations.
     */
    [[nodiscard]] sf::Vector2f get_velocity() const;

    /**
     * @brief Get the current speed of the car based on the current velocity vector.
     *
     * @return Speed magnitude in pixels per second (e.g., "100.f").
     *
     * @note This calculates the scalar speed from the velocity vector before returning it, making it a bit more expensive than simply returning the velocity vector via "get_velocity()".
     */
    [[nodiscard]] float get_speed() const;

    /**
     * @brief Get current steering wheel angle.
     *
     * This is the angle of the steering wheel in degrees, which is used to determine the car's turning radius.
     *
     * @return Steering wheel angle in degrees (e.g., "-180.f", "30.f", "180.f").
     */
    [[nodiscard]] float get_steering_angle() const;

    /**
     * @brief Apply gas to the car immediately.
     *
     * @note Call this every frame to accelerate the car. Do not call it to stop gas.
     */
    virtual void gas();

    /**
     * @brief Apply left foot brake to the car immediately.
     *
     * @note Call this every frame to decelerate the car. Do not call it to stop braking.
     */
    virtual void brake();

    /**
     * @brief Apply left steering to the car with steering wheel emulation (turn the steering wheel left over time until it reaches the maximum angle, at which point it will stay at that angle).
     *
     * @note Call this every frame to steer left. Do not call it to stop steering left and return to center over time.
     */
    void steer_left();

    /**
     * @brief Apply right steering to the car with steering wheel emulation (turn the steering wheel right over time until it reaches the maximum angle, at which point it will stay at that angle).
     *
     * @note Call this every frame to steer right. Do not call it to stop steering right and return to center over time.
     */
    void steer_right();

    /**
     * @brief Apply handbrake (emergency brake) to the car immediately.
     *
     * @note Call this every frame to decelerate the car. Do not call it to stop handbraking.
     */
    void handbrake();

    /**
     * @brief Update the car's physics state over a time interval.
     *
     * @param dt Time passed since the previous frame, in seconds.
     *
     * @note Call this method once per frame before calling "draw()". It will call the internal "apply_physics_step()" function to apply all the physics calculations, such as acceleration, slip, collision, etc.
     */
    virtual void update(const float dt);

    /**
     * @brief Draw the car on the provided render target.
     *
     * @param target Target window where the car will be drawn.
     *
     * @note Call this once per frame, after "update()".
     */
    void draw(sf::RenderTarget &target) const;

    // Disable move semantics
    BaseCar(BaseCar &&) = delete;
    BaseCar &operator=(BaseCar &&) = delete;

    // Disable copy semantics
    BaseCar(const BaseCar &) = delete;
    BaseCar &operator=(const BaseCar &) = delete;

  protected:
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

    /**
     * @brief Random number generator.
     */
    std::mt19937 &rng_;

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
    void apply_physics_step(const float dt);

    /**
     * @brief Last valid position of the car sprite in world coordinates (pixels).
     */
    sf::Vector2f last_position_;

    /**
     * @brief Current velocity of the car in pixels per second.
     */
    sf::Vector2f velocity_;

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
                   const CarConfig &config = CarConfig());  // Use default config

    // Ensure compilation fails if BaseCar's destructor ever stops being virtual
    ~AICar() override = default;

    /**
     * @brief Reset the car's position, rotation, velocity, and steering to initial state.
     *
     * This is useful when rebuilding the track or resetting the game.
     *
     * @note This uses the internal track reference to get the current spawn point. This also resets the current AI waypoint index to 1, skipping the spawn point (index 0) to prevent issues.
     */
    void reset() override;

    /**
     * @brief Update the AI car's steering and throttle/brake decisions based on waypoints over a time interval, then update the car's physics state.
     *
     * @param dt Time passed since the previous frame, in seconds.
     *
     * @note Call this method once per frame before calling "draw()". It will perform AI calculations to determine the best steering and throttle/brake inputs based on the current track and waypoints, then call the base class "update()" method to apply the physics calculations.
     */
    void update(const float dt) override;

  private:
    /**
     * @brief Get a small random variation factor for AI decisions to make them less predictable.
     *
     * @return Float between 0.95 and 1.05 (±5% variation).
     */
    [[nodiscard]] float get_random_variation() const;

    // AI parameters
    float waypoint_reach_factor_ = 0.75f;                // Waypoint reach distance as fraction of tile size
    float collision_distance_ = 0.75f;                   // Collision check distance as fraction of tile size
    float straight_steering_threshold_ = 0.25f;          // Steering threshold on straights (higher = less wiggling)
    float corner_steering_threshold_ = 0.08f;            // Steering threshold in corners (lower = more responsive)
    float minimum_straight_steering_difference_ = 0.1f;  // Minimum heading difference to steer on straights (reduces wiggling)
    float early_corner_turn_distance_ = 1.0f;            // Distance before corner to start turning early (as fraction of tile size)
    float corner_speed_factor_ = 1.2f;                   // Target speed in corners as fraction of tile size
    float straight_speed_factor_ = 3.0f;                 // Target speed on straights as fraction of tile size
    float brake_distance_factor_ = 3.0f;                 // Brake distance as fraction of tile size

    /**
     * @brief Index of the current target waypoint.
     */
    std::size_t current_waypoint_index_number_;
};

}  // namespace core::game
