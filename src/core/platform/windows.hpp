/**
 * @file windows.hpp
 *
 * @brief Microsoft Windows platform-specific functions.
 */

#pragma once

#if defined(_WIN32)

#include <filesystem>  // for std::filesystem

#include <SFML/Graphics.hpp>

namespace core::platform::windows {

/**
 * @brief Add the titlebar icon on Windows using the embedded icon data (must be embedded by CMake first).
 *
 * @param window SFML window to set the titlebar icon for.
 *
 * @throws std::runtime_error if failed to set the icon.
 */
void set_titlebar_icon(const sf::Window &window);

/**
 * @brief Get the path to the local AppData directory on Windows.
 *
 * @return Path to the local AppData directory (e.g., "C:/Users/<username>/AppData/Local").
 *
 * @throws std::runtime_error if failed to retrieve the path.
 */
[[nodiscard]] std::filesystem::path get_local_appdata_directory();

}  // namespace core::platform::windows

#endif
