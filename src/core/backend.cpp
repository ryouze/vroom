/**
 * @file backend.cpp
 */

#include <exception>  // for std::exception
#include <format>     // for std::format
#include <memory>     // for std::unique_ptr, std::make_unique
#include <string>     // for std::string

#include <SFML/Graphics.hpp>
#include <spdlog/spdlog.h>

#include "backend.hpp"
#include "generated.hpp"
#if defined(_WIN32)
#include "platform/windows.hpp"
#endif

namespace core::backend {

/**
 * @brief Private namespace for window configuration.
 *
 * @note This avoids the need for an OOP wrapper around the SFML window.
 */
namespace {

/**
 * @brief Default, sane configuration values for the SFML window.
 *
 * @note Conceptually, these variables are analogous to default constructor parameters in a class.
 */
namespace default_config {

/**
 * @brief Default window width in pixels.
 *
 * @note This is the initial window size; it matches the Steam Deck's resolution.
 */
constexpr unsigned int default_width_px = 1280u;

/**
 * @brief Default window height in pixels.
 *
 * @note This is the initial window size; it matches the Steam Deck's resolution.
 */
constexpr unsigned int default_height_px = 800u;

/**
 * @brief Minimum window width in pixels.
 *
 * @note The window cannot be resized to a width smaller than this value.
 */
constexpr unsigned int minimum_width_px = 800u;

/**
 * @brief Minimum window height in pixels.
 *
 * @note The window cannot be resized to a height smaller than this value.
 */
constexpr unsigned int minimum_height_px = 600u;

/**
 * @brief Default anti-aliasing level.
 *
 * @note If unsupported, SFML will select the closest valid valid option (e.g., 4x, 2x, 0x).
 */
constexpr unsigned int antialiasing_samples = 8u;

/**
 * @brief Default frame rate limit.
 */
constexpr unsigned int fps_limit = 144u;

/**
 * @brief Default key repeat state.
 *
 * @note This is useful for text input, but may not be desired for games. We use press and release events for movement anyway.
 */
constexpr bool key_repeat = false;

}  // namespace default_config

/**
 * @brief Current configuration values for the SFML window, modified at runtime.
 *
 * @note Conceptually, these variables are analogous to (private) member variables in a class.
 */
namespace current_config {

/**
 * @brief Current frame rate limit, since the SFML window doesn't provide a getter.
 *
 * @note Initialized during window creation, updated via "set_fps_limit()", and retrieved via "get_fps_limit()".
 */
unsigned int fps_limit;
}  // namespace current_config

}  // namespace

std::unique_ptr<sf::RenderWindow> create_window()
{
    // Create a window title with the project version
    const std::string window_title = std::format("{} ({})", generated::PROJECT_NAME, generated::PROJECT_VERSION);
    SPDLOG_DEBUG("Creating SFML window '{}'...", window_title);

    // Enable anti-aliasing (fallback to the closest valid match if unsupported)
    sf::ContextSettings custom_settings;
    custom_settings.antiAliasingLevel = default_config::antialiasing_samples;
    SPDLOG_DEBUG("Requesting ('{}', '{}') resolution and '{}x' anti-aliasing...", default_config::default_width_px, default_config::default_height_px, default_config::antialiasing_samples);

    // Create a new SFML window
    std::unique_ptr<sf::RenderWindow> window = std::make_unique<sf::RenderWindow>(
        sf::VideoMode({default_config::default_width_px, default_config::default_height_px}),
        window_title,
        sf::Style::Default,   // Default resizable window
        sf::State::Windowed,  // Floating window
        custom_settings);     // Enable anti-aliasing
    if (const unsigned int actual_aa = window->getSettings().antiAliasingLevel; actual_aa != default_config::antialiasing_samples) [[unlikely]] {
        SPDLOG_WARN("Requested anti-aliasing level '{}x' is not supported, using '{}x' instead!", default_config::antialiasing_samples, actual_aa);
    }
    SPDLOG_DEBUG("SFML window created, applying settings...");

    // Set minimum allowed window size
    window->setMinimumSize(sf::Vector2u{default_config::minimum_width_px, default_config::minimum_height_px});
    SPDLOG_DEBUG("Set minimum window size to ('{}', '{}')!", default_config::minimum_width_px, default_config::minimum_height_px);

    // Set FPS limit
    window->setFramerateLimit(default_config::fps_limit);
    current_config::fps_limit = default_config::fps_limit;  // Also set for the current config
    SPDLOG_DEBUG("Set FPS limit to '{}' FPS!", default_config::fps_limit);

    // Disable key repeat, as we want only one key press to register at a time
    window->setKeyRepeatEnabled(default_config::key_repeat);
    SPDLOG_DEBUG("Set key repeat state to '{}'!", default_config::key_repeat ? "ON" : "OFF");

#if defined(_WIN32)
    SPDLOG_DEBUG("Windows platform detected, setting titlebar icon...");
    // Set window titlebar icon (Windows-only)
    // macOS doesn't have titlebar icons, GNU/Linux is DE-dependent
    try {
        core::platform::windows::set_titlebar_icon(*window);
    }
    catch (const std::exception &e) {
        SPDLOG_WARN("Failed to set Windows titlebar icon: {}", e.what());
    }
    SPDLOG_DEBUG("Set windows titlebar icon!");
#else
    SPDLOG_DEBUG("Non-Windows platform detected, skipping titlebar icon setup!");
#endif

    // Return the created window as a unique pointer
    SPDLOG_DEBUG("Window created successfully, returning it!");
    return window;
}

void set_fps_limit(sf::RenderWindow &window,
                   const unsigned int fps_limit)
{
    current_config::fps_limit = fps_limit;
    window.setFramerateLimit(fps_limit);
    SPDLOG_DEBUG("Set FPS limit to '{}' FPS!", fps_limit);
}

unsigned int get_fps_limit() noexcept
{
    return current_config::fps_limit;
}

}  // namespace core::backend
