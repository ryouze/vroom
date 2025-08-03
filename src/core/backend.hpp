/**
 * @file backend.hpp
 *
 * @brief SFML window abstraction.
 */

#pragma once

#include <format>      // for std::format
#include <functional>  // for std::function
#include <string>      // for std::string

#include <SFML/Graphics.hpp>
// #include <imgui.h>
#include "generated.hpp"
#include "settings.hpp"

namespace core::backend {

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

/**
 * @brief Window display state enumeration.
 */
enum class WindowState {
    Fullscreen,
    Windowed
};

class Window {
  public:
    using event_callback_type = std::function<void(const sf::Event &)>;
    using update_callback_type = std::function<void(const float)>;
    using render_callback_type = std::function<void(sf::RenderWindow &)>;

    /**
     * @brief Construct window using settings from the centralized configuration.
     *
     * The window will be created with the current settings (fullscreen/windowed state, resolution, vsync, fps) from settings::current.
     */
    explicit Window();

    /**
     * @brief Initialize or reinitialize the window with current settings.
     *
     * This method handles both initial window creation and applying changes from the centralized configuration. It will recreate the window if necessary to apply the current settings properly.
     */
    void initialize_with_current_settings();

    // Default destructor
    ~Window() = default;

    // Do not put these simple getters/setters in .cpp
    [[nodiscard]] bool is_fullscreen() const { return this->fullscreen_; }
    [[nodiscard]] bool is_vsync_enabled() const { return this->vsync_enabled_; }
    [[nodiscard]] sf::RenderWindow &raw() { return this->window_; }
    [[nodiscard]] const sf::RenderWindow &raw() const { return this->window_; }
    [[nodiscard]] sf::Vector2u get_resolution() const { return this->window_.getSize(); }
    void request_focus() { this->window_.requestFocus(); }
    void set_view(const sf::View &view) { this->window_.setView(view); }
    void clear(const sf::Color &color) { this->window_.clear(color); }
    template <typename Drawable>
    void draw(const Drawable &drawable) { this->window_.draw(drawable); }
    void display() { this->window_.display(); }
    void close() { this->window_.close(); }

    /**
     * @brief Set window display state and optionally specify video mode for fullscreen.
     *
     * @param state Target window state (fullscreen or windowed).
     * @param mode Video mode to use when switching to fullscreen. Defaults to desktop mode.
     *
     * @note When switching to windowed mode, the mode parameter is ignored and the stored windowed resolution is used.
     */
    void set_window_state(const WindowState state,
                          const sf::VideoMode &mode = sf::VideoMode::getDesktopMode());

    /**
     * @brief Set FPS limit and disable V-sync.
     *
     * @param fps_limit Target frames per second limit.
     *
     * @note Setting an FPS limit will automatically disable V-sync as they are mutually exclusive.
     */
    void set_fps_limit(const unsigned fps_limit);

    /**
     * @brief Enable or disable V-sync.
     *
     * @param enable True to enable V-sync, false to disable.
     *
     * @note Enabling V-sync will automatically disable any FPS limit as they are mutually exclusive.
     */
    void set_vsync(const bool enable);

    /**
     * @brief Apply current settings from the centralized configuration.
     *
     * This method applies the fullscreen, vsync, and fps settings from settings::current to the window.
     * Should be called after configuration is loaded to ensure the window matches the saved settings.
     */
    void apply_current_settings();

    /**
     * @brief Get all available fullscreen video modes.
     *
     * @return Vector of available video modes sorted by resolution (best first).
     */
    [[nodiscard]] static std::vector<sf::VideoMode> get_available_modes();

    /**
     * @brief Run the main application loop with provided callbacks.
     *
     * @param on_event Callback function for handling SFML events.
     * @param on_update Callback function for updating game state (receives delta time).
     * @param on_render Callback function for rendering (receives render window reference).
     *
     * @note The loop continues until the window is closed. Delta time is clamped to prevent extreme values.
     */
    void run(const event_callback_type &on_event,
             const update_callback_type &on_update,
             const render_callback_type &on_render);

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;

    Window(Window &&) = default;
    Window &operator=(Window &&) = default;

  private:
    void create_window(const sf::VideoMode &mode,
                       const sf::State state);
    void apply_sync_settings();

    /**
     * @brief Get the video mode that should be used for the current settings.
     *
     * For fullscreen: returns the mode at settings::current::resolution_idx or desktop mode as fallback.
     * For windowed: returns the windowed resolution from settings.
     *
     * @return The appropriate video mode for current settings.
     */
    [[nodiscard]] sf::VideoMode get_video_mode_for_current_settings() const;

    // Runtime state; always initialize in initializer list
    bool fullscreen_;
    bool vsync_enabled_;
    unsigned frame_limit_;
    sf::Vector2u windowed_resolution_;

    sf::RenderWindow window_;
};

/**
 * @brief Convert an sf::Vector2u to sf::Vector2f.
 *
 * @param vector Vector to convert (e.g., {1280, 800}).
 *
 * @return Converted vector with floating-point values (e.g., {1280.f, 800.f}).
 */
[[nodiscard]] sf::Vector2f to_vector2f(const sf::Vector2u &vector);

}  // namespace core::backend
