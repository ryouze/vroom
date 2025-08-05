/**
 * @file sfx.cpp
 */

#include <algorithm>  // for std::clamp
#include <cmath>      // for std::lerp
#include <cstddef>    // for std::size_t

#include <spdlog/spdlog.h>

#include "settings.hpp"
#include "sfx.hpp"

namespace core::sfx {

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
    // Determine current gear based on speed thresholds
    const std::size_t new_gear = this->determine_gear(speed);
    if (new_gear != this->current_gear_) {
        this->current_gear_ = new_gear;
        // SPDLOG_DEBUG("Engine gear shifted to gear '{}'", this->current_gear_);
    }

    // Calculate fake RPM based on current speed and gear selection
    const float rpm = this->calculate_rpm(speed);

    // Map RPM to pitch using linear interpolation between min and max pitch values
    const float rpm_ratio = std::clamp((rpm - this->min_rpm_) / (this->max_rpm_ - this->min_rpm_), 0.0f, 1.0f);
    float pitch = std::lerp(this->min_pitch_, this->max_pitch_, rpm_ratio);

    // Add smooth gear transition blending to prevent abrupt pitch changes during gear shifts
    if (this->current_gear_ < this->gear_count_ && speed > 0.0f) {
        const float next_gear_threshold = this->gear_shift_speeds_[this->current_gear_];

        // Check if we're approaching a gear shift within the blending zone
        if (speed > (next_gear_threshold - this->gear_blend_zone_) && speed < next_gear_threshold) {
            // Calculate the pitch that the next gear would produce for smooth blending
            const float gear_rpm_range = (this->max_rpm_ - this->min_rpm_) / static_cast<float>(this->gear_count_);
            const float next_gear_rpm = this->min_rpm_ + gear_rpm_range * this->next_gear_rpm_multiplier_;
            const float next_gear_rpm_ratio = std::clamp((next_gear_rpm - this->min_rpm_) / (this->max_rpm_ - this->min_rpm_), 0.0f, 1.0f);
            const float next_gear_pitch = std::lerp(this->min_pitch_, this->max_pitch_, next_gear_rpm_ratio);

            // Blend between current gear pitch and next gear pitch based on position in blending zone
            const float blend_factor = (speed - (next_gear_threshold - this->gear_blend_zone_)) / this->gear_blend_zone_;
            pitch = std::lerp(pitch, next_gear_pitch, blend_factor);
        }
    }

    // When at very low speeds, blend between idle pitch and calculated driving pitch for smooth startup
    if (speed < this->idle_blend_speed_) {
        const float blend_factor = speed / this->idle_blend_speed_;
        pitch = std::lerp(this->idle_pitch_, pitch, blend_factor);
    }

    // Apply the calculated pitch to the engine sound
    this->engine_sound_.setPitch(pitch);

    // Apply volume from settings (already in 0.0-1.0 range, convert to SFML's 0-100 range)
    this->engine_sound_.setVolume(settings::current::engine_volume * 100.0f);
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
    // Start with base idle RPM as the minimum engine speed
    float rpm = this->min_rpm_;

    // Add RPM based on speed within the current gear's operating range
    if (this->current_gear_ > 0 && this->current_gear_ <= this->gear_count_) {
        // Get the speed range for the current gear (minimum speed for this gear)
        const float gear_min_speed = this->gear_shift_speeds_[this->current_gear_ - 1];

        // Determine maximum speed for current gear (either next gear threshold or absolute max speed)
        const float gear_max_speed = (this->current_gear_ < this->gear_count_) ? this->gear_shift_speeds_[this->current_gear_] : this->max_car_speed_;

        // Calculate how far we are through the current gear's speed range (0.0 to 1.0)
        const float speed_in_gear = std::clamp((speed - gear_min_speed) / (gear_max_speed - gear_min_speed), 0.0f, 1.0f);

        // Calculate the RPM range that each gear covers and add proportional RPM
        const float gear_rpm_range = (this->max_rpm_ - this->min_rpm_) / static_cast<float>(this->gear_count_);
        rpm += speed_in_gear * gear_rpm_range * this->gear_overlap_multiplier_;
    }

    // Ensure RPM stays within realistic engine limits
    return std::clamp(rpm, this->min_rpm_, this->max_rpm_);
}

std::size_t EngineSound::determine_gear(const float speed) const
{
    // Find the appropriate gear by checking speed thresholds from highest to lowest gear
    // This ensures we select the highest gear that the current speed can support
    for (std::size_t gear = this->gear_count_; gear >= 1; --gear) {
        if (speed >= this->gear_shift_speeds_[gear - 1]) {
            return gear;
        }
    }

    // Default to first gear for very low speeds or edge cases
    return 1;
}

}  // namespace core::sfx
