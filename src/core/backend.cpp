/**
 * @file backend.cpp
 */

#include <algorithm>  // for std::min
#include <stdexcept>  // for std::runtime_error

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include "backend.hpp"
#include "settings.hpp"

namespace core::backend {

ImGuiContext::ImGuiContext(sf::RenderWindow &window)
    : window_(window)
{
    SPDLOG_DEBUG("Creating ImGui context...");
    if (!ImGui::SFML::Init(window)) [[unlikely]] {
        throw std::runtime_error("Failed to initialize ImGui-SFML");
    }
    SPDLOG_DEBUG("ImGui context created, applying settings...");

    this->disable_ini_saving();
    SPDLOG_DEBUG("Disabled INI file saving!");

    this->apply_theme();
    SPDLOG_DEBUG("Applied ImGui theme!");

    SPDLOG_DEBUG("ImGui context created successfully, exiting constructor!");
}

ImGuiContext::~ImGuiContext()
{
    ImGui::SFML::Shutdown();
}

void ImGuiContext::disable_ini_saving() const
{
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
}

void ImGuiContext::apply_theme() const
{
    // Get the current style
    ImGuiStyle &style = ImGui::GetStyle();

    // Define base constants for consistency
    constexpr float rounding = 8.0f;
    constexpr float padding = 15.0f;
    constexpr float spacing = 8.0f;

    /* Global */
    style.DisabledAlpha = 0.5f;  // Additional alpha multiplier applied by BeginDisabled()

    /* Window */
    style.WindowPadding = {padding, padding};         // Padding within a window
    style.WindowRounding = rounding;                  // Radius of window corners rounding
    style.WindowBorderSize = 0.0f;                    // Thickness of border around windows
    style.WindowMinSize = {20.0f, 20.0f};             // Minimum window size
    style.WindowTitleAlign = {0.5f, 0.5f};            // Alignment for title bar text
    style.WindowMenuButtonPosition = ImGuiDir_Right;  // Position of the window menu button

    /* Child and Popup */
    style.ChildRounding = rounding;  // Radius of child window corners rounding
    style.PopupRounding = rounding;  // Radius of popup window corners rounding

    /* Frame */
    style.FramePadding = {padding, padding / 2.0f};  // Padding within a framed rectangle
    style.FrameRounding = rounding;                  // Radius of frame corners rounding

    /* Item and Cell */
    style.ItemSpacing = {spacing, spacing};         // Spacing between widgets/lines
    style.ItemInnerSpacing = {spacing, spacing};    // Spacing within elements of a composed widget
    style.CellPadding = {padding, padding / 2.0f};  // Padding within a table cell

    /* Indent and Columns */
    style.IndentSpacing = padding;      // Horizontal indentation for tree nodes
    style.ColumnsMinSpacing = spacing;  // Minimum horizontal spacing between two columns

    /* Scrollbar */
    style.ScrollbarRounding = rounding;  // Radius of grab corners for scrollbar

    /* Grab */
    style.GrabMinSize = 5.0f;       // Minimum size of a grab box for slider/scrollbar
    style.GrabRounding = rounding;  // Radius of slider grab corners

    /* Tab */
    style.TabRounding = rounding;  // Radius of upper corners of a tab

    /* Colors: Text */
    style.Colors[ImGuiCol_Text] = {1.0f, 1.0f, 1.0f, 1.0f};
    style.Colors[ImGuiCol_TextDisabled] = {0.275f, 0.318f, 0.451f, 1.0f};

    /* Colors: Window */
    style.Colors[ImGuiCol_WindowBg] = {0.078f, 0.086f, 0.102f, 1.0f};
    style.Colors[ImGuiCol_ChildBg] = {0.093f, 0.100f, 0.116f, 1.0f};
    style.Colors[ImGuiCol_PopupBg] = style.Colors[ImGuiCol_WindowBg];

    /* Colors: Border and Title */
    style.Colors[ImGuiCol_Border] = {0.157f, 0.169f, 0.192f, 1.0f};
    style.Colors[ImGuiCol_BorderShadow] = style.Colors[ImGuiCol_WindowBg];
    style.Colors[ImGuiCol_TitleBg] = {0.047f, 0.055f, 0.071f, 1.0f};
    style.Colors[ImGuiCol_TitleBgActive] = style.Colors[ImGuiCol_TitleBg];
    style.Colors[ImGuiCol_TitleBgCollapsed] = style.Colors[ImGuiCol_WindowBg];

    /* Colors: Menu Bar and Scrollbar Background */
    style.Colors[ImGuiCol_MenuBarBg] = {0.098f, 0.106f, 0.122f, 1.0f};
    style.Colors[ImGuiCol_ScrollbarBg] = {0.047f, 0.055f, 0.071f, 1.0f};

    /* Colors: Frame */
    style.Colors[ImGuiCol_FrameBg] = {0.112f, 0.126f, 0.155f, 1.0f};
    style.Colors[ImGuiCol_FrameBgHovered] = {0.157f, 0.169f, 0.192f, 1.0f};
    style.Colors[ImGuiCol_FrameBgActive] = style.Colors[ImGuiCol_FrameBgHovered];

    /* Colors: Button */
    style.Colors[ImGuiCol_Button] = {0.118f, 0.133f, 0.149f, 1.0f};
    style.Colors[ImGuiCol_ButtonHovered] = {0.182f, 0.190f, 0.197f, 1.0f};
    style.Colors[ImGuiCol_ButtonActive] = {0.155f, 0.155f, 0.155f, 1.0f};

    /* Colors: Header */
    style.Colors[ImGuiCol_Header] = {0.141f, 0.163f, 0.206f, 1.0f};
    style.Colors[ImGuiCol_HeaderHovered] = {0.107f, 0.107f, 0.107f, 1.0f};
    style.Colors[ImGuiCol_HeaderActive] = {0.078f, 0.086f, 0.102f, 1.0f};

    /* Colors: Separator */
    style.Colors[ImGuiCol_Separator] = {0.129f, 0.148f, 0.193f, 1.0f};
    style.Colors[ImGuiCol_SeparatorHovered] = {0.157f, 0.184f, 0.251f, 1.0f};
    style.Colors[ImGuiCol_SeparatorActive] = style.Colors[ImGuiCol_SeparatorHovered];

    /* Colors: Resize Grip */
    style.Colors[ImGuiCol_ResizeGrip] = {0.146f, 0.146f, 0.146f, 1.0f};
    style.Colors[ImGuiCol_ResizeGripHovered] = {0.973f, 1.0f, 0.498f, 1.0f};
    style.Colors[ImGuiCol_ResizeGripActive] = {1.0f, 1.0f, 1.0f, 1.0f};

    /* Colors: Slider */
    style.Colors[ImGuiCol_SliderGrab] = {0.972f, 1.0f, 0.498f, 1.0f};
    style.Colors[ImGuiCol_SliderGrabActive] = {1.0f, 0.795f, 0.498f, 1.0f};

    /* Colors: Tab */
    style.Colors[ImGuiCol_Tab] = {0.078f, 0.086f, 0.102f, 1.0f};
    style.Colors[ImGuiCol_TabHovered] = {0.118f, 0.133f, 0.149f, 1.0f};
    style.Colors[ImGuiCol_TabActive] = style.Colors[ImGuiCol_TabHovered];
    style.Colors[ImGuiCol_TabUnfocused] = {0.078f, 0.086f, 0.102f, 1.0f};
    style.Colors[ImGuiCol_TabUnfocusedActive] = {0.125f, 0.274f, 0.571f, 1.0f};

    /* Colors: Plot */
    style.Colors[ImGuiCol_PlotLines] = {0.522f, 0.600f, 0.702f, 1.0f};
    style.Colors[ImGuiCol_PlotLinesHovered] = {0.039f, 0.980f, 0.980f, 1.0f};
    style.Colors[ImGuiCol_PlotHistogram] = {0.884f, 0.794f, 0.562f, 1.0f};
    style.Colors[ImGuiCol_PlotHistogramHovered] = {0.957f, 0.957f, 0.957f, 1.0f};

    /* Colors: Table */
    style.Colors[ImGuiCol_TableHeaderBg] = {0.047f, 0.055f, 0.071f, 1.0f};
    style.Colors[ImGuiCol_TableBorderStrong] = style.Colors[ImGuiCol_TableHeaderBg];
    style.Colors[ImGuiCol_TableBorderLight] = {0.0f, 0.0f, 0.0f, 1.0f};
    style.Colors[ImGuiCol_TableRowBg] = {0.118f, 0.133f, 0.149f, 1.0f};
    style.Colors[ImGuiCol_TableRowBgAlt] = {0.098f, 0.106f, 0.122f, 1.0f};

    /* Colors: Other */
    style.Colors[ImGuiCol_CheckMark] = {0.973f, 1.0f, 0.498f, 1.0f};
    style.Colors[ImGuiCol_DragDropTarget] = {0.498f, 0.514f, 1.0f, 1.0f};
    style.Colors[ImGuiCol_NavHighlight] = {0.266f, 0.289f, 1.0f, 1.0f};
    style.Colors[ImGuiCol_NavWindowingHighlight] = {0.498f, 0.514f, 1.0f, 1.0f};
    style.Colors[ImGuiCol_NavWindowingDimBg] = {0.196f, 0.176f, 0.545f, 0.502f};
    style.Colors[ImGuiCol_ModalWindowDimBg] = style.Colors[ImGuiCol_NavWindowingDimBg];
}

void ImGuiContext::process_event(const sf::Event &event) const
{
    ImGui::SFML::ProcessEvent(this->window_, event);
}

void ImGuiContext::update(const float dt) const
{
    ImGui::SFML::Update(this->window_, sf::seconds(dt));
}

void ImGuiContext::render() const
{
    ImGui::SFML::Render(this->window_);
}

Window::Window()
    : fullscreen_(settings::current::fullscreen),
      vsync_enabled_(settings::current::vsync),
      frame_limit_(settings::fps::values[static_cast<std::size_t>(settings::current::fps_idx)]),
      windowed_resolution_(settings::defaults::windowed_width, settings::defaults::windowed_height)
{
    SPDLOG_DEBUG("Initializing window with current settings: fullscreen='{}', vsync='{}', fps_idx='{}', resolution_idx='{}'",
                 settings::current::fullscreen, settings::current::vsync, settings::current::fps_idx, settings::current::resolution_idx);

    // Initialize window with current settings
    this->initialize_with_current_settings();
}

void Window::initialize_with_current_settings()
{
    SPDLOG_DEBUG("Initializing window with current settings");

    // Update internal state to match current settings
    this->fullscreen_ = settings::current::fullscreen;
    this->vsync_enabled_ = settings::current::vsync;
    this->frame_limit_ = settings::fps::values[static_cast<std::size_t>(settings::current::fps_idx)];

    // Get the appropriate video mode for current settings
    const sf::VideoMode mode = this->get_video_mode_for_current_settings();
    const sf::State state = this->fullscreen_ ? sf::State::Fullscreen : sf::State::Windowed;

    // Create or recreate the window
    if (this->window_.isOpen()) {
        SPDLOG_DEBUG("Recreating existing window with mode '{}x{}' in '{}' state",
                     mode.size.x, mode.size.y, this->fullscreen_ ? "fullscreen" : "windowed");
        this->window_.close();
    }
    else {
        SPDLOG_DEBUG("Creating new window with mode '{}x{}' in '{}' state",
                     mode.size.x, mode.size.y, this->fullscreen_ ? "fullscreen" : "windowed");
    }

    this->create_window(mode, state);
    SPDLOG_DEBUG("Window initialized successfully");
}

sf::VideoMode Window::get_video_mode_for_current_settings() const
{
    if (this->fullscreen_) {
        const auto available_modes = get_available_modes();
        if (!available_modes.empty() && settings::current::resolution_idx >= 0 &&
            settings::current::resolution_idx < static_cast<int>(available_modes.size())) {
            const sf::VideoMode selected_mode = available_modes[static_cast<std::size_t>(settings::current::resolution_idx)];
            SPDLOG_DEBUG("Using fullscreen mode '{}x{}' from settings (index {})",
                         selected_mode.size.x, selected_mode.size.y, settings::current::resolution_idx);
            return selected_mode;
        }
        else {
            SPDLOG_DEBUG("Invalid resolution index '{}', falling back to desktop mode", settings::current::resolution_idx);
            return sf::VideoMode::getDesktopMode();
        }
    }
    else {
        SPDLOG_DEBUG("Using windowed mode '{}x{}'", this->windowed_resolution_.x, this->windowed_resolution_.y);
        return sf::VideoMode{this->windowed_resolution_};
    }
}

std::vector<sf::VideoMode> Window::get_available_modes()
{
    auto modes = sf::VideoMode::getFullscreenModes();
    SPDLOG_DEBUG("Found '{}' available fullscreen modes", modes.size());
    return modes;
}

void Window::set_window_state(const WindowState state,
                              const sf::VideoMode &mode)
{
    const bool target_fullscreen = (state == WindowState::Fullscreen);

    SPDLOG_DEBUG("Setting window state to '{}' with mode '{}x{}'",
                 target_fullscreen ? "fullscreen" : "windowed", mode.size.x, mode.size.y);

    // Update settings to match the requested change
    settings::current::fullscreen = target_fullscreen;

    // If switching to fullscreen with a specific mode, find and set the resolution index
    if (target_fullscreen) {
        const auto available_modes = get_available_modes();
        for (int i = 0; i < static_cast<int>(available_modes.size()); ++i) {
            if (available_modes[static_cast<std::size_t>(i)].size == mode.size) {
                settings::current::resolution_idx = i;
                SPDLOG_DEBUG("Set resolution index to '{}' for mode '{}x{}'", i, mode.size.x, mode.size.y);
                break;
            }
        }
    }

    // Apply the changes by reinitializing with current settings
    this->initialize_with_current_settings();

    SPDLOG_DEBUG("Window state changed successfully");
}

void Window::set_fps_limit(const unsigned fps_limit)
{
    SPDLOG_DEBUG("Setting FPS limit to '{}' and disabling V-sync", fps_limit);
    // Disallow fps limit when vsync is on
    this->vsync_enabled_ = false;
    this->frame_limit_ = fps_limit;
    this->apply_sync_settings();
}

void Window::set_vsync(const bool enable)
{
    SPDLOG_DEBUG("Setting V-sync to '{}' ({})", enable ? "enabled" : "disabled", enable ? "disabling FPS limit" : "keeping current FPS limit");
    // Disallow vsync when fps limit is set
    this->vsync_enabled_ = enable;
    if (this->vsync_enabled_) {
        this->frame_limit_ = 0;
    }
    this->apply_sync_settings();
}

void Window::run(const event_callback_type &on_event,
                 const update_callback_type &on_update,
                 const render_callback_type &on_render)
{
    SPDLOG_INFO("Starting main window loop!");
    sf::Clock clock;
    while (this->window_.isOpen()) {
        // Allow user of this call to explicitly handle events themselves
        this->window_.handleEvents(on_event);
        // Prevent extreme dt by clamping to 0.1 seconds
        constexpr float dt_max = 0.1f;
        const float dt = std::min(clock.restart().asSeconds(), dt_max);
        on_update(dt);
        on_render(this->window_);
    }
    SPDLOG_INFO("Main window loop ended!");
}

void Window::create_window(const sf::VideoMode &mode,
                           const sf::State state)
{
    SPDLOG_DEBUG("Creating window with video mode '{}x{}' in '{}' state", mode.size.x, mode.size.y, (state == sf::State::Fullscreen) ? "fullscreen" : "windowed");
    const sf::ContextSettings settings{.antiAliasingLevel = settings::defaults::anti_aliasing_level};
    const std::string window_title = std::format("{} ({})", generated::PROJECT_NAME, generated::PROJECT_VERSION);
    this->window_.create(mode, window_title, state, settings);
    this->window_.setMinimumSize(sf::Vector2u{settings::defaults::minimum_width, settings::defaults::minimum_height});
    this->apply_sync_settings();
    SPDLOG_DEBUG("Window created successfully with anti-aliasing level '{}'", settings::defaults::anti_aliasing_level);
}

void Window::apply_sync_settings()
{
    SPDLOG_DEBUG("Applying sync settings with V-sync='{}' and frame_limit='{}'", this->vsync_enabled_, this->frame_limit_);
    this->window_.setVerticalSyncEnabled(this->vsync_enabled_);
    this->window_.setFramerateLimit(this->vsync_enabled_ ? 0u : this->frame_limit_);
}

void Window::apply_current_settings()
{
    SPDLOG_DEBUG("Applying current settings from centralized configuration");

    // Simply reinitialize the window with current settings
    // This is much cleaner than trying to selectively apply changes
    this->initialize_with_current_settings();

    SPDLOG_DEBUG("Applied settings: fullscreen={}, vsync={}, fps_idx={} ({}), resolution_idx={}",
                 settings::current::fullscreen, settings::current::vsync, settings::current::fps_idx,
                 settings::fps::values[static_cast<std::size_t>(settings::current::fps_idx)], settings::current::resolution_idx);
}

sf::Vector2f to_vector2f(const sf::Vector2u &vector)
{
    return {static_cast<float>(vector.x), static_cast<float>(vector.y)};
}

}  // namespace core::backend
