/**
 * @file sfx.cpp
 */

#include <algorithm>  // for std::clamp
#include <cmath>      // for std::lerp
#include <cstddef>    // for std::size_t

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/SoundSource.hpp>
#include <spdlog/spdlog.h>

#include "settings.hpp"
#include "sfx.hpp"

namespace core::sfx {

EngineSound::EngineSound(const sf::SoundBuffer &sound_buffer)
    : engine_sound_(sound_buffer)
{
    this->engine_sound_.setLooping(true);
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

    // Apply volume from settings (already in 0.0-1.0 range, convert to SFML's 0-100 range
    this->engine_sound_.setVolume(std::clamp(settings::current::engine_volume * 100.0f, 0.0f, 100.0f));
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
    return this->engine_sound_.getStatus() == sf::SoundSource::Status::Playing;
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

TireScreechSound::TireScreechSound(const sf::SoundBuffer &sound_buffer)
    : tire_screech_sound_(sound_buffer)
{
    this->tire_screech_sound_.setLooping(true);
    this->tire_screech_sound_.setPitch(this->base_pitch_);
    this->tire_screech_sound_.setVolume(0.0f);  // Start silent

    SPDLOG_DEBUG("TireScreechSound created with base pitch '{}', max pitch '{}', drift threshold '{}', max volume slip velocity '{}'",
                 this->base_pitch_,
                 this->max_pitch_,
                 this->drift_threshold_pixels_per_second_,
                 this->max_volume_slip_velocity_pixels_per_second_);
}

void TireScreechSound::update(const float lateral_slip_velocity, const float car_speed)
{
    // Determine if we should be screeching based on thresholds
    const bool should_screech = (lateral_slip_velocity > this->drift_threshold_pixels_per_second_) &&
                                (car_speed > this->minimum_speed_threshold_pixels_per_second_);

    if (should_screech) {
        // Calculate volume based on lateral slip velocity (0.0 to 1.0)
        const float volume_ratio = std::clamp(lateral_slip_velocity / this->max_volume_slip_velocity_pixels_per_second_, 0.0f, 1.0f);
        this->current_target_volume_ = volume_ratio;

        // Calculate pitch based on lateral slip velocity
        const float pitch_ratio = std::clamp(lateral_slip_velocity / this->max_pitch_slip_velocity_pixels_per_second_, 0.0f, 1.0f);
        const float pitch = std::lerp(this->base_pitch_, this->max_pitch_, pitch_ratio);
        this->tire_screech_sound_.setPitch(pitch);

        // Start playing if not already playing
        if (this->tire_screech_sound_.getStatus() != sf::SoundSource::Status::Playing) {
            this->tire_screech_sound_.play();
        }
    }
    else {
        // Fade out when not drifting
        this->current_target_volume_ = 0.0f;
    }

    // Smooth volume transitions to avoid abrupt start/stop
    this->current_actual_volume_ = std::lerp(this->current_actual_volume_, this->current_target_volume_, this->volume_smoothing_factor_);

    // Apply volume from settings and current calculated volume
    const float final_volume = std::clamp(this->current_actual_volume_ * settings::current::tire_screech_volume * 100.0f, 0.0f, 100.0f);
    this->tire_screech_sound_.setVolume(final_volume);

    // Stop playing if volume is essentially zero to save resources
    if (this->current_actual_volume_ < 0.01f && this->tire_screech_sound_.getStatus() == sf::SoundSource::Status::Playing) {
        this->tire_screech_sound_.stop();
    }
}

void TireScreechSound::stop()
{
    if (this->tire_screech_sound_.getStatus() == sf::SoundSource::Status::Playing) {
        this->tire_screech_sound_.stop();
        this->current_target_volume_ = 0.0f;
        this->current_actual_volume_ = 0.0f;
        SPDLOG_DEBUG("Tire screeching sound stopped");
    }
}

WallHitSound::WallHitSound(const sf::SoundBuffer &sound_buffer)
    : wall_hit_sound_(sound_buffer)
{
    this->wall_hit_sound_.setPitch(this->base_pitch_);
    this->wall_hit_sound_.setVolume(0.0f);  // Start silent, volume set in play()

    SPDLOG_DEBUG("WallHitSound created with base pitch '{}', max pitch '{}', minimum impact speed '{}', max volume impact speed '{}'",
                 this->base_pitch_,
                 this->max_pitch_,
                 this->minimum_impact_speed_pixels_per_second_,
                 this->max_volume_impact_speed_pixels_per_second_);
}

void WallHitSound::play(const float impact_speed)
{
    // Only play if impact speed is above minimum threshold
    if (impact_speed < this->minimum_impact_speed_pixels_per_second_) {
        return;
    }

    // Calculate volume based on impact speed (0.0 to 1.0)
    const float volume_ratio = std::clamp((impact_speed - this->minimum_impact_speed_pixels_per_second_) / (this->max_volume_impact_speed_pixels_per_second_ - this->minimum_impact_speed_pixels_per_second_), 0.0f, 1.0f);

    // Calculate pitch based on impact speed
    const float pitch_ratio = std::clamp(impact_speed / this->max_volume_impact_speed_pixels_per_second_, 0.0f, 1.0f);
    const float pitch = std::lerp(this->base_pitch_, this->max_pitch_, pitch_ratio);

    // Apply volume from settings and calculated ratio
    const float final_volume = std::clamp(settings::current::wall_hit_volume * volume_ratio * 100.0f, 0.0f, 100.0f);
    this->wall_hit_sound_.setVolume(final_volume);
    this->wall_hit_sound_.setPitch(pitch);
    this->wall_hit_sound_.play();  // play() restarts if already playing in SFML 3

    // SPDLOG_DEBUG("Wall hit sound played with impact speed '{}', volume ratio '{}', final volume '{}', pitch '{}'", impact_speed, volume_ratio, final_volume, pitch);
}

UiSound::UiSound(const sf::SoundBuffer &ok_sound_buffer,
                 const sf::SoundBuffer &other_sound_buffer)
    : ok_sound_(ok_sound_buffer),
      other_sound_(other_sound_buffer)
{
    SPDLOG_DEBUG("UiSound created with '{}' volume", settings::current::ui_volume * 100.0f);
}

void UiSound::play_ok()
{
    // Apply volume from settings (convert from 0.0-1.0 to 0-100 range for SFML)
    const float final_volume = std::clamp(settings::current::ui_volume * 100.0f, 0.0f, 100.0f);
    this->ok_sound_.setVolume(final_volume);
    this->ok_sound_.play();  // play() restarts if already playing in SFML 3

    // SPDLOG_DEBUG("UI 'ok' sound played with volume '{}'", final_volume);
}

void UiSound::play_other()
{
    const float final_volume = std::clamp(settings::current::ui_volume * 100.0f, 0.0f, 100.0f);
    this->other_sound_.setVolume(final_volume);
    this->other_sound_.play();  // play() restarts if already playing in SFML 3

    // SPDLOG_DEBUG("UI 'other' sound played with volume '{}'", final_volume);
}

}  // namespace core::sfx
