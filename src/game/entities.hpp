/**
 * @file entities.hpp
 *
 * @brief Cars.
 */

#pragma once

#include <cstddef>  // for std::size_t
#include <random>   // for std::mt19937

#include <SFML/Graphics.hpp>

#include "core/world.hpp"  // We depend on the Track class for car collision detection and waypoints

namespace game::entities {

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
 * @brief Struct that contains the essential state information of a car.
 *
 * This groups commonly accessed properties together to simplify the Car API.
 */
struct CarState final {
    /**
     * @brief Current position in world coordinates (pixels).
     */
    sf::Vector2f position;

    /**
     * @brief Current velocity in pixels per second.
     */
    sf::Vector2f velocity;

    /**
     * @brief Current speed magnitude in pixels per second.
     */
    float speed;

    /**
     * @brief Current steering wheel angle in degrees.
     */
    float steering_angle;

    /**
     * @brief Current control mode (Player or AI).
     */
    CarControlMode control_mode;

    /**
     * @brief Current waypoint index for race position tracking.
     */
    std::size_t waypoint_index;

    /**
     * @brief Current accumulated drift score.
     */
    float drift_score;
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
                 const core::world::Track &track,
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
     * @brief Get the current state of the car.
     *
     * @return CarState struct containing position, velocity, speed, steering angle, control mode, waypoint index, and drift score.
     *
     * @note This replaces individual getter methods and provides all commonly needed car information in one call.
     */
    [[nodiscard]] CarState get_state() const;

    /**
     * @brief Set the control mode at runtime.
     *
     * @param control_mode New control mode (Player or AI).
     *
     * @note When switching to AI mode, the current waypoint index is reset to ensure proper navigation.
     */
    void set_control_mode(const CarControlMode control_mode);

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
     * @brief Update waypoint tracking for race position regardless of control mode.
     *
     * @param dt Time passed since the previous frame, in seconds.
     *
     * @note This is called automatically during "update()" for all cars to maintain accurate race position tracking.
     */
    void update_waypoint_tracking(const float dt);

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
    const core::world::Track &track_;

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
    CarInput current_input_;

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
    float drift_score_;

    /**
     * @brief Distance factor for waypoint reach detection used by both AI and waypoint tracking.
     *
     * Increase = reach waypoints from farther away, decrease = must get closer to reach waypoints.
     */
    static constexpr float waypoint_reach_factor_ = 0.65f;

    /**
     * @brief Random variation parameters for waypoint tracking consistency.
     *
     * These provide consistent random variations for waypoint reach distance calculations.
     */
    static constexpr float random_variation_minimum_ = 0.8f;
    static constexpr float random_variation_maximum_ = 1.2f;

    /**
     * @brief Time accumulator for AI update throttling.
     *
     * This tracks elapsed time since last AI behavior update to limit AI calculations to maximum 30Hz for performance.
     */
    float ai_update_timer_ = 0.0f;

    /**
     * @brief Target interval for AI updates in seconds (1/30 = ~0.0333 seconds for 30Hz).
     *
     * AI behavior will only be recalculated when ai_update_timer_ exceeds this interval.
     */
    static constexpr float ai_accumulation_ = 1.0f / 30.0f;
};

}  // namespace game::entities
