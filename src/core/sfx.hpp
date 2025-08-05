/**
 * @file sfx.hpp
 *
 * @brief Car sound effects.
 */

#pragma once

#include <array>    // for std::array
#include <cstddef>  // for std::size_t

#include <SFML/Audio.hpp>

namespace core::sfx {

/**
 * @brief Class that manages engine sound playback based on car speed.
 *
 * Simulates a 5-gear transmission by calculating fake RPM from speed and adjusting pitch accordingly.
 */
class EngineSound final {
  public:
    /**
     * @brief Construct a new EngineSound object.
     *
     * @param sound_buffer Reference to the engine sound buffer to be played in a loop.
     */
    explicit EngineSound(const sf::SoundBuffer &sound_buffer);

    /**
     * @brief Default destructor.
     */
    ~EngineSound() = default;

    /**
     * @brief Update engine sound based on current car speed.
     *
     * @param speed Current car speed in pixels per second.
     *
     * @note This calculates fake RPM, determines the appropriate gear, and adjusts pitch and volume accordingly.
     */
    void update(const float speed);

    /**
     * @brief Start playing the engine sound loop.
     */
    void start();

    /**
     * @brief Stop playing the engine sound.
     */
    void stop();

    /**
     * @brief Check if the engine sound is currently playing.
     *
     * @return True if the sound is playing, false otherwise.
     */
    [[nodiscard]] bool is_playing() const;

    // Allow move semantics
    EngineSound(EngineSound &&) = default;
    EngineSound &operator=(EngineSound &&) = default;

    // Disable copy semantics
    EngineSound(const EngineSound &) = delete;
    EngineSound &operator=(const EngineSound &) = delete;

  private:
    /**
     * @brief Calculate fake RPM based on current speed and gear.
     *
     * @param speed Current car speed in pixels per second.
     *
     * @return Calculated RPM value.
     */
    [[nodiscard]] float calculate_rpm(const float speed) const;

    /**
     * @brief Determine the current gear based on speed.
     *
     * @param speed Current car speed in pixels per second.
     *
     * @return Current gear (1-5).
     */
    [[nodiscard]] std::size_t determine_gear(const float speed) const;

    /**
     * @brief SFML Sound object for engine sound playback.
     */
    sf::Sound engine_sound_;

    /**
     * @brief Current gear (1-5).
     */
    std::size_t current_gear_;

    /**
     * @brief Number of gears in the transmission.
     */
    static constexpr std::size_t gear_count_ = 5;

    /**
     * @brief Idle pitch when car is not moving.
     */
    static constexpr float idle_pitch_ = 0.8f;

    /**
     * @brief Minimum pitch multiplier for driving engine sound.
     */
    static constexpr float min_pitch_ = 1.0f;

    /**
     * @brief Maximum pitch multiplier for redline engine sound.
     */
    static constexpr float max_pitch_ = 2.7f;

    /**
     * @brief Minimum RPM value for idle engine.
     */
    static constexpr float min_rpm_ = 800.0f;

    /**
     * @brief Maximum RPM value for redline engine.
     */
    static constexpr float max_rpm_ = 7500.0f;

    /**
     * @brief Speed threshold for smooth idle-to-driving pitch transition.
     */
    static constexpr float idle_blend_speed_ = 450.0f;

    /**
     * @brief Blending zone distance before gear shift for smooth transitions.
     */
    static constexpr float gear_blend_zone_ = 50.0f;

    /**
     * @brief Multiplier for next gear RPM calculation during gear transitions.
     */
    static constexpr float next_gear_rpm_multiplier_ = 1.2f * 0.2f;

    /**
     * @brief Maximum car speed in pixels per second.
     */
    static constexpr float max_car_speed_ = 2500.0f;

    /**
     * @brief Overlap multiplier between gears for smoother RPM transitions.
     */
    static constexpr float gear_overlap_multiplier_ = 1.2f;

    /**
     * @brief Speed thresholds for gear shifts (pixels per second).
     *
     * @brief Better balanced gear shifts that don't change too frequently.
     */
    static constexpr std::array<float, 5> gear_shift_speeds_ = {
        0.0f,     // 1st gear: 0 - 500 px/s
        500.0f,   // 2nd gear: 500 - 1000 px/s
        1000.0f,  // 3rd gear: 1000 - 1500 px/s
        1500.0f,  // 4th gear: 1500 - 2000 px/s
        2000.0f   // 5th gear: 2000 - 2500 px/s (max speed)
    };
};

/**
 * @brief Class that manages tire screeching sound playback based on car drift.
 *
 * Plays tire screeching sound when the car is drifting, with volume and pitch adjusted based on lateral slip velocity.
 */
class TireScreechSound final {
  public:
    /**
     * @brief Construct a new TireScreechSound object.
     *
     * @param sound_buffer Reference to the tire screeching sound buffer to be played in a loop.
     */
    explicit TireScreechSound(const sf::SoundBuffer &sound_buffer);

    /**
     * @brief Default destructor.
     */
    ~TireScreechSound() = default;

    /**
     * @brief Update tire screeching sound based on current car state.
     *
     * @param lateral_slip_velocity Current lateral slip velocity magnitude in pixels per second.
     * @param car_speed Current car speed in pixels per second.
     *
     * @note This adjusts sound volume and pitch based on lateral slip velocity.
     */
    void update(const float lateral_slip_velocity, const float car_speed);

    /**
     * @brief Stop playing the tire screeching sound.
     */
    void stop();

    /**
     * @brief Check if the tire screeching sound is currently playing.
     *
     * @return True if the sound is playing, false otherwise.
     */
    [[nodiscard]] bool is_playing() const;

    // Allow move semantics
    TireScreechSound(TireScreechSound &&) = default;
    TireScreechSound &operator=(TireScreechSound &&) = default;

    // Disable copy semantics
    TireScreechSound(const TireScreechSound &) = delete;
    TireScreechSound &operator=(const TireScreechSound &) = delete;

  private:
    /**
     * @brief SFML Sound object for tire screeching sound playback.
     */
    sf::Sound tire_screech_sound_;

    /**
     * @brief Minimum lateral slip velocity required to trigger tire screeching sound in pixels per second.
     */
    static constexpr float drift_threshold_pixels_per_second_ = 150.0f;

    /**
     * @brief Minimum car speed required for tire screeching in pixels per second.
     */
    static constexpr float minimum_speed_threshold_pixels_per_second_ = 250.0f;

    /**
     * @brief Lateral slip velocity at which tire screeching reaches maximum volume in pixels per second.
     */
    static constexpr float max_volume_slip_velocity_pixels_per_second_ = 300.0f;

    /**
     * @brief Lateral slip velocity at which tire screeching reaches maximum pitch in pixels per second.
     */
    static constexpr float max_pitch_slip_velocity_pixels_per_second_ = 400.0f;

    /**
     * @brief Base pitch for tire screeching sound.
     */
    static constexpr float base_pitch_ = 0.8f;

    /**
     * @brief Maximum pitch multiplier for tire screeching sound.
     */
    static constexpr float max_pitch_ = 1.5f;

    /**
     * @brief Volume fade-in/fade-out smoothing factor per frame.
     */
    static constexpr float volume_smoothing_factor_ = 0.1f;

    /**
     * @brief Current target volume for smooth transitions.
     */
    float current_target_volume_ = 0.0f;

    /**
     * @brief Current actual volume for smooth transitions.
     */
    float current_actual_volume_ = 0.0f;
};

}  // namespace core::sfx
