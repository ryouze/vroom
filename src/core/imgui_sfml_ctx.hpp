/**
 * @file imgui_sfml_ctx.hpp
 *
 * @brief Wrapper for ImGui-SFML that provides a RAII manager.
 */

#pragma once

#include <SFML/Graphics.hpp>

namespace core::imgui_sfml_ctx {

/**
 * @brief RAII class that manages the ImGui-SFML context. Put it somewhere before the main loop to ensure proper initialization and cleanup.
 *
 * On construction, the class initializes the ImGui-SFML context with the specified SFML window, disables INI file saving, and applies the Moonlight theme.
 * On destruction, it automatically shuts down ImGui-SFML.
 */
class ImGuiContext final {
  public:
    /**
     * @brief Construct a new ImGuiContext object.
     *
     * @param window Reference to an SFML Window used for ImGui-SFML setup, event handling, and rendering.
     *
     * @throws std::runtime_error if ImGui-SFML failed to initialize.
     */
    explicit ImGuiContext(sf::RenderWindow &window);

    /**
     * @brief Destroy the ImGuiContext object.
     *
     * This automatically cleans up the ImGui context.
     */
    ~ImGuiContext();

    /**
     * @brief Forward a SFML event to ImGui.
     *
     * This method passes the provided event, enabling user interaction with ImGui widgets (e.g., text input, clicks).
     *
     * @param event SFML event to process.
     *
     * @note Call this method once per event in the main loop before updating or rendering ImGui.
     */
    void process_event(const sf::Event &event) const;

    /**
     * @brief Update ImGui's internal state for the current frame.
     *
     * This includes updating ImGui time, processing queued inputs, and preparing for rendering in the upcoming frame.
     *
     * @param dt Time passed since the previous frame, in seconds.
     *
     * @note Call this method once per frame after handling events and before calling "render()".
     */
    void update(const float dt) const;

    /**
     * @brief Render ImGui draw data onto the provided window.
     *
     * Ensure that this is the last ImGui-related call; any custom widgets should be drawn before this method is called.
     *
     * @note Call this once per frame, after "update()" and after all custom ImGui widgets have been drawn.
     */
    void render() const;

    // Delete copy operations
    ImGuiContext(const ImGuiContext &) = delete;
    ImGuiContext &operator=(const ImGuiContext &) = delete;

    // Delete move operations
    ImGuiContext(ImGuiContext &&) = delete;
    ImGuiContext &operator=(ImGuiContext &&) = delete;

  private:
    /**
     * @brief Disable saving ImGui's configuration to an INI file.
     *
     * ImGui by default saves UI layout and other state to an INI file. This method ensures no .ini file is written.
     */
    void disable_ini_saving() const;

    /**
     * @brief Apply the Moonlight theme to ImGui.
     *
     * Author: deathsu/madam-herta
     * Source: https://github.com/Madam-Herta/Moonlight
     *
     * @note Some styling values were adjusted slightly for consistency with more recent ImGui updates, but the overall palette and style remain the same.
     */
    void apply_theme() const;

    /**
     * @brief Target window where the ImGui will be drawn.
     */
    sf::RenderWindow &window_;
};

}  // namespace core::backend
