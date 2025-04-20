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
 * @brief Lightweight struct that holds references to the texture tiles used to build the track.
 *
 * This struct serves as a strongly-typed parameter for the "Track" class and holds references to textures.
 * The caller is responsible for ensuring that these textures remain valid for the lifetime of the "Track" instance.
 * It is assumed that all textures are square and of the same size (e.g., 256x256) for uniform scaling.
 *
 * @note This struct is marked as "final" to prevent inheritance.
 */
struct TrackTiles final {
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
 * @brief Class that manages the race track, including its textures, configuration, positioning, and rendering.
 *
 * On construction, the class builds the track using the provided textures and config.
 *
 * @note This class is marked as "final" to prevent inheritance.
 */
class Track final {
  public:
    /**
     * @brief Construct a new Track object.
     *
     * On construction, the track is built using the provided textures and config.
     *
     * @param tiles Tiles struct containing the textures. It is assumed that all textures are square (e.g., 256x256) for uniform scaling. The caller is responsible for ensuring that these textures remain valid for the lifetime of the Track.
     * @param rng Instance of a random number generator (e.g., std::mt19937) used for generating random detours.
     * @param config Configuration struct containing the track configuration (default: "TrackConfig()").
     */
    explicit Track(const TrackTiles &tiles,
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
    [[nodiscard]] bool is_on_track(const sf::Vector2f &world_position) const;  // TODO: Remove this, let Car class check collisions on its own

    // [[nodiscard]] std::vector<sf::FloatRect> get_bounds() const  // TODO: Store this in a variable to avoid recalculating every frame
    // {
    //     std::vector<sf::FloatRect> bounds;
    //     for (const auto &sprite : this->sprites_) {
    //         bounds.emplace_back(sprite.getGlobalBounds());
    //     }
    //     return bounds;
    // }
    // TODO: Add a waypoint system to help AI cars navigate the track

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
    const TrackTiles &tiles_;

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
     * @brief Finish point of the track.
     *
     * This is the position of the finish line tile. It can be used as a spawn point for cars.
     */
    sf::Vector2f finish_point_;
};

struct CarSettings {
    // Linear accelerations/decelerations (px/s²)
    float forward_acceleration_px_s2 = 700.0f;     // throttle acceleration
    float brake_deceleration_px_s2 = 950.0f;       // brake deceleration
    float handbrake_deceleration_px_s2 = 2200.0f;  // handbrake deceleration
    float engine_brake_px_s2 = 80.0f;              // passive engine braking

    // Speed limits (px/s)
    float max_speed_px_s = 2500.0f;  // terminal velocity

    // Steering rates (deg/s)
    float steering_turn_rate_deg_s = 520.0f;    // degrees/sec when turning
    float steering_center_rate_deg_s = 580.0f;  // degrees/sec to center wheel

    // Steering limits
    float max_steering_angle_deg = 180.0f;  // wheel limit

    // How sharply the car can turn at low vs. high speed
    float turn_factor_zero_speed = 1.0f;  // full turn at zero speed
    float turn_factor_full_speed = 0.8f;  // reduced turn at top speed

    // Side slip damping (per second)
    float side_damping_per_sec = 12.0f;  // lateral slip damping

    // Collision bounce speed retention factor
    float collision_speed_factor = 0.3f;  // bounce-back retention

    constexpr CarSettings() = default;
};

//----------------------------------------------------------------------
// Base Car: handles update loop, physics, steering, and bounce.
//----------------------------------------------------------------------

class Car {
  public:
    Car(const sf::Texture &texture,
        const CarSettings &settings,
        const sf::Vector2f spawn_position = {0.0f, -300.0f})
        : settings_(settings),
          spawn_position_(spawn_position)
    {
        // Create rectangle shape matching texture size
        sf::Vector2u tex_size = texture.getSize();
        this->shape_.setSize({static_cast<float>(tex_size.x), static_cast<float>(tex_size.y)});
        // Center origin so rotation occurs around center
        this->shape_.setOrigin(this->shape_.getSize() / 2.0f);
        this->shape_.setPosition(this->spawn_position_);
        this->shape_.setTexture(&texture);

        // Initialize last-known-good position
        this->last_position_ = this->spawn_position_;
    }

    virtual ~Car() = default;

    // Advance physics and input-propagation by dt seconds
    void update(float dt)
    {
        // Save for potential bounce-back
        this->last_position_ = this->shape_.getPosition();

        // Derived-class handles accelerate/brake/steer flags
        this->handle_control(dt);

        // If no input, apply passive engine braking
        float speed = std::hypot(
            this->velocity_.x,
            this->velocity_.y);
        if (!this->input_applied_ && speed > 0.01f) [[likely]] {
            float new_speed = speed - this->settings_.engine_brake_px_s2 * dt;
            new_speed = std::max(new_speed, 0.0f);
            this->velocity_ *= (new_speed / speed);
        }

        this->limit_speed();
        this->apply_side_damping(dt);
        this->apply_steering(dt);

        // Finally, rotate sprite and move by velocity
        this->shape_.setRotation(this->heading_);
        this->shape_.move(this->velocity_ * dt);

        // Clear input flags
        this->input_applied_ = false;
        this->steering_left_ = false;
        this->steering_right_ = false;
    }

    // Bounce back to last known good position, invert velocity
    void bounce_back()
    {
        this->shape_.setPosition(this->last_position_);
        this->velocity_ = -this->velocity_ * this->settings_.collision_speed_factor;
    }

    // Draw car onto target
    void draw(sf::RenderTarget &target) const
    {
        target.draw(this->shape_);
    }

    // Reset to spawn, zero velocity and steering
    void reset(const sf::Vector2f &spawn_position)
    {
        this->spawn_position_ = spawn_position;
        this->shape_.setPosition(spawn_position);
        this->velocity_ = {0.0f, 0.0f};
        this->heading_ = sf::degrees(0.0f);
        this->steering_angle_deg_ = 0.0f;
    }

    // Directly set position
    void set_position(const sf::Vector2f &position)
    {
        this->shape_.setPosition(position);
    }

    [[nodiscard]] sf::Vector2f get_position() const
    {
        return this->shape_.getPosition();
    }

    [[nodiscard]] sf::Vector2f get_velocity() const
    {
        return this->velocity_;
    }

    [[nodiscard]] sf::Angle get_heading() const
    {
        return this->heading_;
    }

    [[nodiscard]] float get_steering_angle_deg() const
    {
        return this->steering_angle_deg_;
    }

    [[nodiscard]] const CarSettings &get_settings() const
    {
        return this->settings_;
    }
    [[nodiscard]] CarSettings &get_settings()
    {
        return this->settings_;
    }

  protected:
    // Implement in subclass: set flags for accelerate/brake/steer
    virtual void handle_control(float dt) = 0;

    // Throttle input
    void accelerate(float dt)
    {
        float rad = this->heading_.asRadians();
        this->velocity_.x += std::cos(rad) * this->settings_.forward_acceleration_px_s2 * dt;
        this->velocity_.y += std::sin(rad) * this->settings_.forward_acceleration_px_s2 * dt;
        this->input_applied_ = true;
    }

    // Brake input
    void brake(float dt)
    {
        float speed = std::hypot(this->velocity_.x, this->velocity_.y);
        if (speed > 0.01f) {
            float dir_x = this->velocity_.x / speed;
            float dir_y = this->velocity_.y / speed;
            float reduction = std::min(
                this->settings_.brake_deceleration_px_s2 * dt,
                speed);
            this->velocity_.x -= dir_x * reduction;
            this->velocity_.y -= dir_y * reduction;
        }
        this->input_applied_ = true;
    }

    // Handbrake input
    void handbrake(float dt)
    {
        float speed = std::hypot(this->velocity_.x, this->velocity_.y);
        if (speed > 0.01f) {
            float dir_x = this->velocity_.x / speed;
            float dir_y = this->velocity_.y / speed;
            float new_speed = speed - this->settings_.handbrake_deceleration_px_s2 * dt;
            if (new_speed < 0.1f) {
                this->velocity_ = {0.0f, 0.0f};
            }
            else {
                this->velocity_.x = dir_x * new_speed;
                this->velocity_.y = dir_y * new_speed;
            }
        }
        this->input_applied_ = true;
    }

    // Turn wheel left
    void steer_left(float dt)
    {
        this->steering_angle_deg_ -= this->settings_.steering_turn_rate_deg_s * dt;
        this->steering_left_ = true;
    }

    // Turn wheel right
    void steer_right(float dt)
    {
        this->steering_angle_deg_ += this->settings_.steering_turn_rate_deg_s * dt;
        this->steering_right_ = true;
    }

  private:
    // Clamp to max speed
    void limit_speed()
    {
        float speed = std::hypot(this->velocity_.x, this->velocity_.y);
        if (speed > this->settings_.max_speed_px_s) [[likely]] {
            float scale = this->settings_.max_speed_px_s / speed;
            this->velocity_ *= scale;
        }
    }

    // Reduce lateral slip each update
    void apply_side_damping(float dt)
    {
        float rad = this->heading_.asRadians();
        sf::Vector2f forward(std::cos(rad), std::sin(rad));
        float fwd_spd = forward.x * this->velocity_.x + forward.y * this->velocity_.y;
        sf::Vector2f fwd_vel = forward * fwd_spd;
        sf::Vector2f lat_vel = this->velocity_ - fwd_vel;
        float factor = std::clamp(
            this->settings_.side_damping_per_sec * dt,
            0.0f, 1.0f);
        this->velocity_ = fwd_vel + lat_vel * (1.0f - factor);
    }

    // Adjust heading by steering angle, modulated by speed
    void apply_steering(float dt)
    {
        // Auto-center if no input
        if (!this->steering_left_ && !this->steering_right_) {
            float center_amt = this->settings_.steering_center_rate_deg_s * dt;
            if (this->steering_angle_deg_ > center_amt)
                this->steering_angle_deg_ -= center_amt;
            else if (this->steering_angle_deg_ < -center_amt)
                this->steering_angle_deg_ += center_amt;
            else
                this->steering_angle_deg_ = 0.0f;
        }

        // Clamp steering angle
        this->steering_angle_deg_ = std::clamp(
            this->steering_angle_deg_,
            -this->settings_.max_steering_angle_deg,
            this->settings_.max_steering_angle_deg);

        // Blend responsiveness by speed ratio
        float speed = std::hypot(this->velocity_.x, this->velocity_.y);
        float ratio = (speed > 0.0f)
                          ? (speed / this->settings_.max_speed_px_s)
                          : 0.0f;
        float turn_factor = this->settings_.turn_factor_zero_speed * (1.0f - ratio) + this->settings_.turn_factor_full_speed * ratio;
        if (speed < 5.0f)
            turn_factor *= (speed / 5.0f);

        this->heading_ += sf::degrees(
            this->steering_angle_deg_ * turn_factor * dt);
    }

    sf::RectangleShape shape_;  // visual representation
    sf::Vector2f velocity_;     // current movement vector
    sf::Angle heading_;         // orientation angle
    float steering_angle_deg_ = 0.0f;

    bool input_applied_ = false;  // throttle/brake applied?
    bool steering_left_ = false;  // steering input flags
    bool steering_right_ = false;

    CarSettings settings_;         // immutable behavior parameters
    sf::Vector2f spawn_position_;  // initial spawn point
    sf::Vector2f last_position_;   // last safe position for bounce
};

//----------------------------------------------------------------------
// PlayerCar: maps explicit boolean flags to Car actions.
//----------------------------------------------------------------------

class PlayerCar final : public Car {
  public:
    using Car::Car;

    void set_input(bool accelerate_flag,
                   bool brake_flag,
                   bool turn_left_flag,
                   bool turn_right_flag,
                   bool handbrake_flag)
    {
        this->accelerate_flag_ = accelerate_flag;
        this->brake_flag_ = brake_flag;
        this->turn_left_flag_ = turn_left_flag;
        this->turn_right_flag_ = turn_right_flag;
        this->handbrake_flag_ = handbrake_flag;
    }

  protected:
    void handle_control(float dt) override
    {
        if (this->accelerate_flag_) {
            this->accelerate(dt);
        }
        if (this->brake_flag_) {
            this->brake(dt);
        }
        if (this->handbrake_flag_) {
            this->handbrake(dt);
        }
        if (this->turn_left_flag_) {
            this->steer_left(dt);
        }
        if (this->turn_right_flag_) {
            this->steer_right(dt);
        }
    }

  private:
    bool accelerate_flag_ = false;
    bool brake_flag_ = false;
    bool turn_left_flag_ = false;
    bool turn_right_flag_ = false;
    bool handbrake_flag_ = false;
};

//----------------------------------------------------------------------
// AiCar: simple AI steering and throttle decisions toward a target.
//----------------------------------------------------------------------

class AiCar final : public Car {
  public:
    using Car::Car;

    void set_target(const sf::Vector2f &target_position,
                    float min_distance,
                    float max_distance)
    {
        this->target_position_ = target_position;
        this->min_distance_ = min_distance;
        this->max_distance_ = max_distance;
    }

    void set_update_interval(float interval_seconds)
    {
        this->update_interval_ = interval_seconds;
    }

  protected:
    void handle_control(float dt) override
    {
        this->decide_turning();
        this->time_accumulator_ += dt;
        if (this->time_accumulator_ >= this->update_interval_) {
            this->time_accumulator_ = 0.0f;
            this->decide_throttle();
        }
        if (this->throttle_flag_) {
            this->accelerate(dt);
        }
        if (this->brake_flag_) {
            this->brake(dt);
        }
        if (this->steer_left_flag_) {
            this->steer_left(dt);
        }
        if (this->steer_right_flag_) {
            this->steer_right(dt);
        }
    }

  private:
    // Steering toward target: compare angles and set left/right flags
    void decide_turning()
    {
        this->steer_left_flag_ = false;
        this->steer_right_flag_ = false;
        sf::Vector2f diff = this->target_position_ - this->get_position();
        float desired = std::atan2(diff.y, diff.x);
        float current = this->get_heading().asRadians();
        float delta = std::remainder(desired - current, 2 * std::numbers::pi_v<float>);
        if (delta < -0.02f)
            this->steer_left_flag_ = true;
        else if (delta > 0.02f)
            this->steer_right_flag_ = true;
    }

    // Throttle/brake based on distance to target
    void decide_throttle()
    {
        this->throttle_flag_ = false;
        this->brake_flag_ = false;
        sf::Vector2f diff = this->target_position_ - this->get_position();
        float distance = std::hypot(diff.x, diff.y);
        if (distance > this->max_distance_)
            this->throttle_flag_ = true;
        else if (distance < this->min_distance_)
            this->brake_flag_ = true;
    }

    sf::Vector2f target_position_ = {0.0f, 0.0f};
    float min_distance_ = 120.0f;
    float max_distance_ = 300.0f;

    float update_interval_ = 0.3f;
    float time_accumulator_ = 0.0f;

    bool throttle_flag_ = false;
    bool brake_flag_ = false;
    bool steer_left_flag_ = false;
    bool steer_right_flag_ = false;
};

}  // namespace core::game
