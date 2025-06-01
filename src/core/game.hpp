/**
 * @file game.hpp
 *
 * @brief Game world map abstractions.
 */

#pragma once

#include <cmath>    // for std::abs
#include <cstddef>  // for std::size_t
#include <random>   // for std::mt19937
#include <vector>   // for std::vector

#include <SFML/Graphics.hpp>

namespace core::game {

/**
 * @brief Struct that represents the configurable parameters of the track. Invalid values will be clamped to reasonable defaults.
 */
struct TrackConfig final {
    /**
     * @brief Number of horizontal tiles, i.e., width (e.g., "8").
     *
     * @note If detours are enabled, +1 tile may be added to each side when a detour occurs, increasing the effective total width, even though the core "horizontal_count" remains unchanged.
     */
    std::size_t horizontal_count = 7;

    /**
     * @brief Number of vertical tiles, i.e., height (e.g., "6").
     */
    std::size_t vertical_count = 7;

    /**
     * @brief Size of each tile in pixels (e.g., "256").
     *
     * @note This determines the rendered size of each track tile and does not depend on the source texture size, which will be scaled to the size provided here accordingly.
     *
     * @details The default texture size is 128x128px, so we are scaling it up 12 times.
     */
    std::size_t size_px = 1536;

    /**
     * @brief Probability in the range [0.0, 1.0] that a detour bubble will be generated on each vertical edge segment.
     *
     * @note A value of 0.0 disables detours entirely, while 1.0 maximizes detour frequency. Horizontal edges always remain straight regardless of this setting.
     */
    float detour_probability = 0.7f;

    /**
     * @brief Equality comparison operator with epsilon-based float comparison.
     */
    [[nodiscard]] bool operator==(const TrackConfig &other) const noexcept
    {
        constexpr float epsilon = 1e-6f;
        return this->horizontal_count == other.horizontal_count &&
               this->vertical_count == other.vertical_count &&
               this->size_px == other.size_px &&
               std::abs(this->detour_probability - other.detour_probability) < epsilon;
    }
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
         * ```
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
         * ```
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
         * ```
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
         * ```
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
         * ```
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
         * ```
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
     * This performs a simple rectangular collision check against all track tile bounds using pre-cached collision rectangles. While it's technically possible to go outside curved tile shapes visually, this approach is simple and fast.
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
     * @return Copy of the configuration with invalid values clamped to safe values (e.g., "horizontal_count" and "vertical_count" must be at least "3", "size_px" must be at least "256", and "detour_probability" must be in range [0.0, 1.0]).
     *
     * @note Always use this during construction or configuration changes to prevent catastrophic errors.
     */
    [[nodiscard]] static TrackConfig validate_config(const TrackConfig &config);

    /**
     * @brief Build the track layout using the current configuration and textures.
     *
     * This creates the complete track by placing corner tiles, horizontal/vertical edge tiles, and optional detour bubbles. It also generates collision bounds, waypoint sequences for AI navigation, and identifies the finish line position. Random detour bubbles are inserted on vertical edges according to detour probability, while horizontal edges remain straight.
     *
     * @note This is marked as private, because we only want to build the track on construction and explicit config changes.
     */
    void build();

    /**
     * @brief Tile textures used for building the track.
     *
     * This contains references to all track tile textures including curves, straights, and finish line.
     *
     * @details A complete copy is stored to prevent segfaults if the original texture references become invalid.
     */
    const Textures tiles_;

    /**
     * @brief Random number generator used for procedural track generation.
     *
     * This determines random detour placement along vertical track edges according to the configured detour probability. Each edge segment uses this RNG to decide whether to generate a detour bubble.
     */
    std::mt19937 &rng_;

    /**
     * @brief Current validated track configuration.
     *
     * This contains all track parameters after validation and clamping. Values are guaranteed to be within safe ranges to prevent crashes or invalid track generation.
     */
    TrackConfig config_;

    /**
     * @brief Collection of sprite objects for each tile in the track layout.
     *
     * This contains all track tile sprites positioned and scaled according to the track configuration. Sprites are created during track building and ready for rendering each frame.
     */
    std::vector<sf::Sprite> sprites_;

    /**
     * @brief Axis-aligned bounding rectangles used for collision detection against each sprite.
     *
     * This contains pre-cached collision bounds for efficient track boundary checking. Each rectangle corresponds to a sprite in the "sprites_" vector.
     *
     * @note This ensures that we don't need to call "getGlobalBounds()" on each sprite every time we want to check for collisions.
     */
    std::vector<sf::FloatRect> collision_bounds_;

    /**
     * @brief Ordered sequence of waypoints defining the AI navigation path around the track.
     *
     * This contains waypoints placed at each tile center with appropriate driving type classification (straight or corner). AI cars follow this sequence to navigate around the track efficiently.
     */
    std::vector<TrackWaypoint> waypoints_;

    /**
     * @brief Center position of the finish-line sprite, used as a spawn point for vehicles.
     *
     * This contains the world coordinates of the finish line tile center where cars are initially placed. Set during track building when the finish line tile is positioned on the top edge.
     */
    sf::Vector2f finish_point_;
};

/**
 * @brief Struct that represents the configurable parameters of the car.
 */
struct CarConfig final {
    /**
     * @brief The rate at which throttle input accelerates the car, expressed in pixels per second squared.
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

    /**
     * @brief Speed threshold below which the car is considered stopped for physics calculations.
     *
     * This prevents jitter and unnecessary calculations at very low speeds.
     */
    float stopped_speed_threshold_pixels_per_second = 0.01f;

    /**
     * @brief Steering wheel angle threshold below which auto-centering snaps to zero.
     *
     * This prevents oscillation when the steering wheel is nearly centered.
     */
    float steering_autocenter_epsilon_degrees = 0.1f;

    /**
     * @brief Minimum forward speed required for sprite rotation during steering.
     *
     * Below this threshold, the car sprite will not rotate even if steering input is applied.
     */
    float minimum_speed_for_rotation_pixels_per_second = 1.0f;
};

/**
 * @brief Struct that represents unified input state for both keyboard and controller.
 *
 * Supports both digital input (keyboard: -1, 0, 1) and analog input (controller: continuous values).
 */
struct CarInput final {
    /**
     * @brief Gas/throttle input value. Range [0.0, 1.0] for analog, or 0.0/1.0 for digital.
     */
    float throttle = 0.0f;

    /**
     * @brief Brake input value. Range [0.0, 1.0] for analog, or 0.0/1.0 for digital.
     */
    float brake = 0.0f;

    /**
     * @brief Steering input value. Range [-1.0, 1.0] for analog, or -1.0/0.0/1.0 for digital.
     * Negative values steer left, positive values steer right.
     */
    float steering = 0.0f;

    /**
     * @brief Handbrake input. Range [0.0, 1.0] for analog, or 0.0/1.0 for digital.
     */
    float handbrake = 0.0f;
};

/**
 * @brief Enum for car control modes.
 */
enum class CarControlMode {
    /**
     * @brief Player control mode - responds to keyboard/controller input.
     */
    Player,

    /**
     * @brief AI control mode - follows waypoints automatically.
     */
    AI
};

/**
 * @brief Unified car class that supports both player and AI control modes.
 *
 * Provides core physics simulation, rendering, collision handling, and AI navigation in a single class.
 * Control mode can be switched both at construction time and during runtime.
 *
 * On construction, the car is initialized with a texture, position, and control mode.
 */
class Car final {
  public:
    /**
     * @brief Construct a new Car object.
     *
     * @param texture Reference to the SFML texture used for the car sprite. This is expected to be around 71x131 pixels.
     * @param rng Reference to a random number generator used for random decision making (e.g., collision bounces).
     * @param track Reference to the race track object for boundary, spawnpoint and waypoint information.
     * @param control_mode Initial control mode (Player or AI).
     * @param config Configuration parameters controlling acceleration, braking, steering, and collision behavior.
     *
     * @note This uses the internal track reference to place the car at the track's spawn point.
     */
    explicit Car(const sf::Texture &texture,
                 std::mt19937 &rng,
                 const Track &track,
                 const CarControlMode control_mode = CarControlMode::Player,
                 const CarConfig &config = CarConfig());  // Use default config

    /**
     * @brief Reset the car's position, rotation, velocity, and steering to initial state.
     *
     * This is useful when rebuilding the track or resetting the game.
     *
     * @note This uses the internal track reference to place the car at the track's spawn point.
     */
    void reset();

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
     * @brief Get the current control mode.
     *
     * @return Current control mode (Player or AI).
     */
    [[nodiscard]] CarControlMode get_control_mode() const;

    /**
     * @brief Set the control mode at runtime.
     *
     * @param control_mode New control mode (Player or AI).
     *
     * @note When switching to AI mode, the current waypoint index is reset to ensure proper navigation.
     */
    void set_control_mode(const CarControlMode control_mode);

    /**
     * @brief Get the current waypoint index for race position tracking.
     *
     * @return Current waypoint index that the car is targeting or has passed.
     *
     * @note This is useful for determining race positions relative to other cars.
     */
    [[nodiscard]] std::size_t get_current_waypoint_index() const;

    /**
     * @brief Get the current drift score accumulated by this car.
     *
     * @return Current drift score as a float value.
     */
    [[nodiscard]] float get_drift_score() const;

    /**
     * @brief Apply unified input for both keyboard and controller.
     *
     * @param input Input values for throttle, brake, steering, and handbrake.
     *
     * @note Only effective in Player mode. Supports both digital (-1/0/1) and analog input values.
     */
    void apply_input(const CarInput &input);

    /**
     * @brief Update the car's physics state over a time interval.
     *
     * @param dt Time passed since the previous frame, in seconds.
     *
     * @note Call this method once per frame before calling "draw()". In AI mode, this will also perform AI calculations to determine steering and throttle/brake inputs based on waypoints.
     */
    void update(const float dt);

    /**
     * @brief Draw the car on the provided render target.
     *
     * @param target Target window where the car will be drawn.
     *
     * @note Call this once per frame, after "update()".
     */
    void draw(sf::RenderTarget &target) const;

    // Disable move semantics
    Car(Car &&) = delete;
    Car &operator=(Car &&) = delete;

    // Disable copy semantics
    Car(const Car &) = delete;
    Car &operator=(const Car &) = delete;

  private:
    /**
     * @brief Apply physics step to the car - combines all forces, slip, and collisions.
     *
     * This processes the member input flags, then runs the entire physics pipeline.
     *
     * @param dt Time passed since the previous frame, in seconds.
     *
     * @note Always call this as the final step in the update loop from "update()", before drawing the car via "draw()".
     */
    void apply_physics_step(const float dt);

    /**
     * @brief Update AI behavior - handles waypoint navigation and control decisions.
     *
     * @param dt Time passed since the previous frame, in seconds.
     *
     * @note This is called automatically during "update()" when in AI mode.
     */
    void update_ai_behavior(const float dt);

    /**
     * @brief Car sprite object for rendering. Also used for motion and rotation.
     *
     * The sprite handles visual representation, position tracking, and rotation for physics calculations.
     */
    sf::Sprite sprite_;

    /**
     * @brief Reference to the race track for collision detection and waypoint data.
     *
     * Used for boundary checking, collision detection, spawn point retrieval, and AI waypoint navigation.
     */
    const Track &track_;

    /**
     * @brief Configuration of the car.
     *
     * Contains all performance parameters including acceleration, braking, steering sensitivity, and collision behavior.
     */
    const CarConfig config_;

    /**
     * @brief Random number generator.
     *
     * Used for collision bounce randomization and AI decision variation to prevent deterministic behavior.
     */
    std::mt19937 &rng_;

    /**
     * @brief Current control mode (Player or AI).
     *
     * Determines whether the car responds to player input or follows AI waypoint navigation.
     */
    CarControlMode control_mode_;

    /**
     * @brief Last valid position of the car sprite in world coordinates (pixels).
     *
     * Stored for collision recovery when the car moves off-track and needs to be restored to a legal position.
     */
    sf::Vector2f last_position_;

    /**
     * @brief Current velocity of the car in pixels per second.
     *
     * Two-dimensional velocity vector used for physics calculations including acceleration, braking, and collision response.
     */
    sf::Vector2f velocity_;

    /**
     * @brief Current input values for analog/digital control.
     *
     * Keyboard provides digital values (0.0 or 1.0), controller provides analog values (0.0 to 1.0).
     * Updated by apply_input() and used directly by physics calculations.
     */
    CarInput current_input_ = {};

    /**
     * @brief Current steering wheel angle in degrees. This emulates a steering wheel via "steer_left()" and "steer_right()". If they are not called, the steering wheel will return to center over time.
     *
     * Positive values turn the car right, negative values turn it left. Auto-centers when no steering input is active.
     */
    float steering_wheel_angle_;

    /**
     * @brief Index of the current target waypoint for AI navigation.
     *
     * Tracks which waypoint the AI car is currently navigating towards in the track's waypoint sequence.
     * Also used for race position tracking regardless of control mode.
     */
    std::size_t current_waypoint_index_number_;

    /**
     * @brief Current accumulated drift score for this car.
     *
     * Score increases based on drift angle magnitude, speed, and duration while the car is sliding sideways.
     */
    float drift_score_ = 0.0f;
};

}  // namespace core::game
