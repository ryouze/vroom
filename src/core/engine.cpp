/**
 * @file engine.cpp
 */

#include <algorithm>  // for std::clamp
#include <cmath>      // for std::fmod

#include <spdlog/spdlog.h>

#include "engine.hpp"

namespace core::engine {

EngineSound::EngineSound(const sf::SoundBuffer &sound_buffer)
    : engine_sound_(sound_buffer),
      current_gear_(1)
{
    this->engine_sound_.setLooping(true);             // SFML3 uses setLooping instead of setLoop
    this->engine_sound_.setPitch(this->idle_pitch_);  // Start with idle pitch

    SPDLOG_DEBUG("EngineSound created with idle pitch '{}', min pitch '{}', max pitch '{}', min RPM '{}', max RPM '{}'",
                 this->idle_pitch_,
                 this->min_pitch_,
                 this->max_pitch_,
                 this->min_rpm_,
                 this->max_rpm_);
}
void EngineSound::update(const float speed)
{
    // Determine current gear based on speed
    const int new_gear = this->determine_gear(speed);
    if (new_gear != this->current_gear_) {
        this->current_gear_ = new_gear;
        SPDLOG_DEBUG("Engine gear shifted to gear '{}'", this->current_gear_);
    }

    // Calculate fake RPM based on speed and gear
    const float rpm = this->calculate_rpm(speed);

    // Map RPM to pitch (linear interpolation between min and max pitch)
    const float rpm_ratio = std::clamp((rpm - this->min_rpm_) / (this->max_rpm_ - this->min_rpm_), 0.0f, 1.0f);
    float pitch = this->min_pitch_ + rpm_ratio * (this->max_pitch_ - this->min_pitch_);

    // Add smooth gear transition blending
    if (this->current_gear_ < 5 && speed > 0.0f) {
        const float next_gear_threshold = this->gear_shift_speeds_[this->current_gear_];
        const float gear_blend_zone = 50.0f;  // 30 px/s blending zone before gear shift (reduced from 100)

        // Check if we're approaching a gear shift
        if (speed > (next_gear_threshold - gear_blend_zone) && speed < next_gear_threshold) {
            // Calculate next gear's pitch for blending
            const float next_gear_rpm_range = (this->max_rpm_ - this->min_rpm_) / 5.0f;
            const float next_gear_rpm = this->min_rpm_ + next_gear_rpm_range * 1.2f * 0.2f;  // Start of next gear RPM
            const float next_gear_rpm_ratio = std::clamp((next_gear_rpm - this->min_rpm_) / (this->max_rpm_ - this->min_rpm_), 0.0f, 1.0f);
            const float next_gear_pitch = this->min_pitch_ + next_gear_rpm_ratio * (this->max_pitch_ - this->min_pitch_);

            // Blend between current and next gear pitch
            const float blend_factor = (speed - (next_gear_threshold - gear_blend_zone)) / gear_blend_zone;
            pitch = pitch * (1.0f - blend_factor) + next_gear_pitch * blend_factor;
        }
    }

    // When at very low speeds, blend between idle pitch and calculated pitch
    if (speed < this->idle_blend_speed_) {                           // Smooth transition zone
        const float blend_factor = speed / this->idle_blend_speed_;  // 0.0 at speed=0, 1.0 at idle_blend_speed_
        pitch = this->idle_pitch_ * (1.0f - blend_factor) + pitch * blend_factor;
    }

    // Set the pitch
    this->engine_sound_.setPitch(pitch);
}

void EngineSound::start()
{
    if (this->engine_sound_.getStatus() != sf::SoundSource::Status::Playing) {
        this->engine_sound_.play();
        SPDLOG_DEBUG("Engine sound started");
    }
}

void EngineSound::stop()
{
    if (this->engine_sound_.getStatus() == sf::SoundSource::Status::Playing) {
        this->engine_sound_.stop();
        SPDLOG_DEBUG("Engine sound stopped");
    }
}

bool EngineSound::is_playing() const
{
    return this->engine_sound_.getStatus() == sf::SoundSource::Status::Playing;  // SFML3 uses sf::SoundSource::Status
}

float EngineSound::calculate_rpm(const float speed) const
{
    // Base RPM calculation: idle RPM + speed-based component
    float rpm = this->min_rpm_;

    // Add RPM based on speed within current gear range
    if (this->current_gear_ > 0 && this->current_gear_ <= 5) {  // Changed to 5 gears
        const float gear_min_speed = this->gear_shift_speeds_[this->current_gear_ - 1];
        const float gear_max_speed = (this->current_gear_ < 5) ? this->gear_shift_speeds_[this->current_gear_] : 2500.0f;  // Max car speed

        // Calculate speed within current gear range (0.0 to 1.0)
        const float speed_in_gear = std::clamp((speed - gear_min_speed) / (gear_max_speed - gear_min_speed), 0.0f, 1.0f);

        // Calculate gear-specific RPM range
        const float gear_rpm_range = (this->max_rpm_ - this->min_rpm_) / 5.0f;  // Divide RPM range across 5 gears
        rpm += speed_in_gear * gear_rpm_range * 1.2f;                           // 1.2f for overlap between gears
    }

    return std::clamp(rpm, this->min_rpm_, this->max_rpm_);
}

int EngineSound::determine_gear(const float speed) const
{
    // Find the appropriate gear based on speed thresholds
    for (int gear = 5; gear >= 1; --gear) {  // Changed to 5 gears
        if (speed >= this->gear_shift_speeds_[gear - 1]) {
            return gear;
        }
    }
    return 1;  // Default to first gear for very low speeds
}

}  // namespace core::engine
