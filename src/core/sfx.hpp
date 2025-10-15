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
    static constexpr float idle_pitch_ = 0.8F;

    /**
     * @brief Minimum pitch multiplier for driving engine sound.
     */
    static constexpr float min_pitch_ = 1.0F;

    /**
     * @brief Maximum pitch multiplier for redline engine sound.
     */
    static constexpr float max_pitch_ = 2.7F;

    /**
     * @brief Minimum RPM value for idle engine.
     */
    static constexpr float min_rpm_ = 800.0F;

    /**
     * @brief Maximum RPM value for redline engine.
     */
    static constexpr float max_rpm_ = 7500.0F;

    /**
     * @brief Speed threshold for smooth idle-to-driving pitch transition.
     */
    static constexpr float idle_blend_speed_ = 450.0F;

    /**
     * @brief Blending zone distance before gear shift for smooth transitions.
     */
    static constexpr float gear_blend_zone_ = 50.0F;

    /**
     * @brief Multiplier for next gear RPM calculation during gear transitions.
     */
    static constexpr float next_gear_rpm_multiplier_ = 1.2F * 0.2F;

    /**
     * @brief Maximum car speed in pixels per second.
     */
    static constexpr float max_car_speed_ = 2500.0F;

    /**
     * @brief Overlap multiplier between gears for smoother RPM transitions.
     */
    static constexpr float gear_overlap_multiplier_ = 1.2F;

    /**
     * @brief Speed thresholds for gear shifts (pixels per second).
     *
     * @brief Better balanced gear shifts that don't change too frequently.
     */
    static constexpr std::array<float, 5> gear_shift_speeds_ = {
        0.0F,     // 1st gear: 0 - 500 px/s
        500.0F,   // 2nd gear: 500 - 1000 px/s
        1000.0F,  // 3rd gear: 1000 - 1500 px/s
        1500.0F,  // 4th gear: 1500 - 2000 px/s
        2000.0F   // 5th gear: 2000 - 2500 px/s (max speed)
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
    static constexpr float drift_threshold_pixels_per_second_ = 150.0F;

    /**
     * @brief Minimum car speed required for tire screeching in pixels per second.
     */
    static constexpr float minimum_speed_threshold_pixels_per_second_ = 250.0F;

    /**
     * @brief Lateral slip velocity at which tire screeching reaches maximum volume in pixels per second.
     */
    static constexpr float max_volume_slip_velocity_pixels_per_second_ = 300.0F;

    /**
     * @brief Lateral slip velocity at which tire screeching reaches maximum pitch in pixels per second.
     */
    static constexpr float max_pitch_slip_velocity_pixels_per_second_ = 400.0F;

    /**
     * @brief Base pitch for tire screeching sound.
     */
    static constexpr float base_pitch_ = 0.8F;

    /**
     * @brief Maximum pitch multiplier for tire screeching sound.
     */
    static constexpr float max_pitch_ = 1.5F;

    /**
     * @brief Volume fade-in/fade-out smoothing factor per frame.
     */
    static constexpr float volume_smoothing_factor_ = 0.1F;

    /**
     * @brief Current target volume for smooth transitions.
     */
    float current_target_volume_ = 0.0F;

    /**
     * @brief Current actual volume for smooth transitions.
     */
    float current_actual_volume_ = 0.0F;
};

/**
 * @brief Class that manages wall hit sound playback when the car collides with track boundaries.
 *
 * Plays a wall hit sound effect with volume adjusted based on collision impact speed.
 */
class WallHitSound final {
  public:
    /**
     * @brief Construct a new WallHitSound object.
     *
     * @param sound_buffer Reference to the wall hit sound buffer to be played.
     */
    explicit WallHitSound(const sf::SoundBuffer &sound_buffer);

    /**
     * @brief Default destructor.
     */
    ~WallHitSound() = default;

    /**
     * @brief Play the wall hit sound with volume based on collision impact speed.
     *
     * @param impact_speed Speed at which the collision occurred in pixels per second.
     *
     * @note Volume is scaled based on impact speed, with minimum threshold to avoid playing at very low speeds.
     */
    void play(const float impact_speed);

    // Allow move semantics
    WallHitSound(WallHitSound &&) = default;
    WallHitSound &operator=(WallHitSound &&) = default;

    // Disable copy semantics
    WallHitSound(const WallHitSound &) = delete;
    WallHitSound &operator=(const WallHitSound &) = delete;

  private:
    /**
     * @brief SFML Sound object for wall hit sound playback.
     */
    sf::Sound wall_hit_sound_;

    /**
     * @brief Minimum impact speed required to trigger wall hit sound in pixels per second.
     */
    static constexpr float minimum_impact_speed_pixels_per_second_ = 100.0F;

    /**
     * @brief Impact speed at which wall hit sound reaches maximum volume in pixels per second.
     */
    static constexpr float max_volume_impact_speed_pixels_per_second_ = 1000.0F;

    /**
     * @brief Base pitch for wall hit sound.
     */
    static constexpr float base_pitch_ = 0.4F;

    /**
     * @brief Maximum pitch multiplier for wall hit sound at high impact speeds.
     */
    static constexpr float max_pitch_ = 1.0F;
};

/**
 * @brief Class that manages UI sound playback for menu interactions.
 *
 * Plays UI sound effects for various interface interactions like button clicks and menu navigation.
 */
class UiSound final {
  public:
    /**
     * @brief Construct a new UiSound object.
     *
     * @param ok_sound_buffer Reference to the 'ok' sound buffer to be played for confirmations.
     * @param other_sound_buffer Reference to the 'other' sound buffer to be played for misc actions.
     */
    UiSound(const sf::SoundBuffer &ok_sound_buffer,
            const sf::SoundBuffer &other_sound_buffer);

    /**
     * @brief Default destructor.
     */
    ~UiSound() = default;

    /**
     * @brief Play the 'ok' sound for confirmations and most UI interactions.
     *
     * Used for button clicks, confirmations, and general UI navigation.
     */
    void play_ok();

    /**
     * @brief Play the 'other' sound for miscellaneous actions.
     *
     * Used for actions like opening/closing menus, toggling pause, and other special actions.
     */
    void play_other();

    // Allow move semantics
    UiSound(UiSound &&) = default;
    UiSound &operator=(UiSound &&) = default;

    // Disable copy semantics
    UiSound(const UiSound &) = delete;
    UiSound &operator=(const UiSound &) = delete;

  private:
    /**
     * @brief SFML Sound object for 'ok' sound playback.
     */
    sf::Sound ok_sound_;

    /**
     * @brief SFML Sound object for 'other' sound playback.
     */
    sf::Sound other_sound_;
};

}  // namespace core::sfx
