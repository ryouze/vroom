/**
 * @file game.test.cpp
 */

#include <snitch/snitch.hpp>

#include "game/entities.hpp"

TEST_CASE("CarConfig default values are reasonable", "[src][game][entities.hpp]")
{
    const game::entities::CarConfig config;
    CHECK(config.throttle_acceleration_rate_pixels_per_second_squared > 0.0f);
    CHECK(config.brake_deceleration_rate_pixels_per_second_squared > 0.0f);
    CHECK(config.handbrake_deceleration_rate_pixels_per_second_squared > config.brake_deceleration_rate_pixels_per_second_squared);
    CHECK(config.maximum_movement_pixels_per_second > 0.0f);
    CHECK(config.steering_turn_rate_degrees_per_second > 0.0f);
    CHECK(config.steering_autocenter_rate_degrees_per_second > 0.0f);
    CHECK(config.maximum_steering_angle_degrees > 0.0f);
    CHECK(config.steering_sensitivity_at_zero_speed >= config.steering_sensitivity_at_maximum_speed);
    CHECK(config.lateral_slip_damping_coefficient_per_second > 0.0f);
    CHECK(config.collision_velocity_retention_ratio >= 0.0f);
    CHECK(config.collision_velocity_retention_ratio <= 1.0f);
}
