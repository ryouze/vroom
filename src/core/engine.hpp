/**
 * @file engine.hpp
 *
 * @brief Car engine sounds.
 */

#pragma once

#include <SFML/Audio.hpp>

namespace core::engine {

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
    [[nodiscard]] int determine_gear(const float speed) const;

    /**
     * @brief SFML Sound object for engine sound playback.
     */
    sf::Sound engine_sound_;

    /**
     * @brief Current gear (1-5).
     */
    int current_gear_;

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
     * @brief Speed thresholds for gear shifts (pixels per second).
     *
     * @brief Better balanced gear shifts that don't change too frequently.
     */
    static constexpr float gear_shift_speeds_[5] = {
        0.0f,     // 1st gear: 0 - 500 px/s
        500.0f,   // 2nd gear: 500 - 1000 px/s
        1000.0f,  // 3rd gear: 1000 - 1500 px/s
        1500.0f,  // 4th gear: 1500 - 2000 px/s
        2000.0f   // 5th gear: 2000 - 2500 px/s (max speed)
    };
};

}  // namespace core::engine
