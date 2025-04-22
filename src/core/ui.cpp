/**
 * @file ui.cpp
 */

#include <algorithm>    // for std::clamp
#include <cstdint>      // for std::uint32_t
#include <format>       // for std::format
#include <stdexcept>    // for std::runtime_error
#include <string>       // for std::string
#include <type_traits>  // for std::underlying_type_t
#include <utility>      // for std::move

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include "ui.hpp"

namespace core::ui {

namespace {

/**
 * @brief Compute the pivot point based on the specified corner.
 *
 * @param corner Corner of the window where the speedometer will be displayed.
 *
 * @return Pivot point where (0, 0) is the top-left corner and (1, 1) is the bottom-right corner.
 */
[[nodiscard]] inline constexpr ImVec2 compute_pivot(const Corner corner)
{
    switch (corner) {
    case Corner::TopLeft:
        return {0.f, 0.f};
    case Corner::TopRight:
        return {1.f, 0.f};
    case Corner::BottomLeft:
        return {0.f, 1.f};
    case Corner::BottomRight:
        [[fallthrough]];  // Falling through is intended behavior
    default:
        return {1.f, 1.f};
    }
}

/**
 * @brief Compute the offset based on the specified pivot and padding.
 *
 * @param pivot Pivot point where (0, 0) is the top-left corner and (1, 1) is the bottom-right corner.
 * @param padding Padding value in pixels (default: "10.f").
 *
 * @return Offset, which is the distance from the pivot point to the edge of the window.
 */
[[nodiscard]] inline constexpr ImVec2 compute_offset(const ImVec2 pivot,
                                                     const float padding = 10.f)
{
    return {pivot.x == 0.f ? padding : -padding,
            pivot.y == 0.f ? padding : -padding};
}

/**
 * @brief Base flags for overlay windows - non-interactive, non-movable, and non-resizable.
 *
 * @note More flags can be appended on a per-window basis via bitwise OR (e.g., "base_overlay_flags | ImGuiWindowFlags_NoBackground").
 */
constexpr ImGuiWindowFlags base_overlay_flags =
    // ImGuiWindowFlags_AlwaysAutoResize |       // Always resize the window to fit its contents
    // ImGuiWindowFlags_NoResize |               // Prevent the window from being resized
    ImGuiWindowFlags_NoBringToFrontOnFocus |  // Prevent forcing the window to the front when focused
    ImGuiWindowFlags_NoCollapse |             // Disable collapsing the window
    ImGuiWindowFlags_NoFocusOnAppearing |     // Do not auto-focus when appearing
    ImGuiWindowFlags_NoInputs |               // Disable all mouse/keyboard interaction
    ImGuiWindowFlags_NoMove |                 // Prevent the window from being moved
    ImGuiWindowFlags_NoTitleBar;              // Remove the window title bar

/**
 * @brief Get the scaled size of a rectangle based on the smaller of the two dimensions of the window. Only the width is scaled by the ratio, while the height is calculated based on the aspect ratio.
 *
 * This is useful for creating rectangles that dynamically scale with the window size.
 *
 * @param window_width Window width (e.g., "1920").
 * @param window_height Window height (e.g., "1080").
 * @param aspect_ratio Aspect ratio (e.g., "16.0f / 9.0f"). You can also use "30.f / 200.f" for 30px height and 200px width.
 * @param scale_ratio Scale ratio (e.g., "0.2f").
 *
 * @return Size of the rectangle (e.g., "384x216").
 *
 * @note By taking the smaller of the two dimensions, we ensure that ultrawide monitors and other weird aspect ratios don't break the UI.
 */
[[nodiscard]] inline ImVec2 get_scaled_rectangle_size(const unsigned window_width,
                                                      const unsigned window_height,
                                                      const float aspect_ratio,
                                                      const float scale_ratio)
{
    // Pick the smaller of the screen dimensions
    const float shorter = static_cast<float>((window_width < window_height)
                                                 ? window_width
                                                 : window_height);
    // Scale it by the ratio
    const float width = shorter * scale_ratio;
    // Return the the scaled with, with the height calculated based on the aspect ratio, perfect for a menu
    return {width, width * aspect_ratio};
}

/**
 * @brief Get the size of a square based on the smaller of the two dimensions of the window. Both width and height are scaled by the ratio.
 *
 * This is useful for creating square ImGui elements that dynamically scale with the window size.
 *
 * @param window_width Window width (e.g., "1920").
 * @param window_height Window height (e.g., "1080").
 * @param scale_ratio Scale ratio (e.g., "0.2f").
 *
 * @return Size of the square (e.g., "384x384").
 *
 * @note By taking the smaller of the two dimensions, we ensure that ultrawide monitors and other weird aspect ratios don't break the UI.
 */
[[nodiscard]] inline constexpr ImVec2 get_scaled_square_size(const unsigned window_width,
                                                             const unsigned window_height,
                                                             const float scale_ratio)
{
    // Pick the smaller of the screen dimensions
    const float shorter = static_cast<float>((window_width < window_height)
                                                 ? window_width
                                                 : window_height);
    // Scale it by the ratio
    const float side_length = shorter * scale_ratio;
    // Return the scaled size as a square, perfect for a box, such as a minimap
    return {side_length, side_length};
}

}  // namespace

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

FpsCounter::FpsCounter(sf::RenderTarget &window,
                       const Corner corner)
    : enabled(false),  // Disable by default
      window_(window),
      pivot_(compute_pivot(corner)),
      accumulation_(0.0f),
      frames_(0),
      fps_(144)  // Default value, will overwritten immediately by internal calculations
{
    SPDLOG_DEBUG("FPS counter created at corner '{}', set pivot point to ('{}', '{}') successfully, exiting constructor!",
                 static_cast<std::underlying_type_t<Corner>>(corner),
                 this->pivot_.x,
                 this->pivot_.y);
}

void FpsCounter::update_and_draw(const float dt)
{
    // If disabled, do nothing
    if (!this->enabled) {
        return;
    }

    // The dt is already clamped to [0.001, 0.1] in the main loop, so we can safely use it here

    this->update(dt);
    this->draw();
}

void FpsCounter::update(const float dt)
{
    // Accumulate the delta time and increment the frame count
    this->accumulation_ += dt;
    ++this->frames_;

    // If the accumulated time exceeds the update rate, calculate the FPS
    if (this->accumulation_ >= this->update_rate_) {
        this->fps_ = static_cast<std::uint32_t>(this->frames_ / this->accumulation_);
        this->frames_ = 0;
        this->accumulation_ -= this->update_rate_;  // Keep any overshoot
    }
}

void FpsCounter::draw() const
{
    // Get the current window size
    const auto [width, height] = this->window_.getSize();

    // Use pivot_.x and pivot_.y to decide how much of the window size to add
    ImGui::SetNextWindowPos({this->pivot_.x * static_cast<float>(width),
                             this->pivot_.y * static_cast<float>(height)},
                            ImGuiCond_Always,
                            this->pivot_);  // Corner of the window
    ImGui::Begin("FPS Counter",
                 nullptr,
                 base_overlay_flags |
                     ImGuiWindowFlags_NoBackground  // Disable background drawing (make it transparent)
    );
    ImGui::Text("FPS: %u", this->fps_);
    ImGui::End();
}

Speedometer::Speedometer(sf::RenderTarget &window,
                         const Corner corner)
    : enabled(true),  // Enable by default
      window_(window),
      pivot_(compute_pivot(corner)),
      offset_(compute_offset(this->pivot_))
{
    SPDLOG_DEBUG("Speedometer created at corner '{}', set pivot point to ('{}', '{}') and padding offset to ('{}', '{}') px successfully, exiting constructor!",
                 static_cast<std::underlying_type_t<Corner>>(corner),
                 this->pivot_.x,
                 this->pivot_.y,
                 this->offset_.x,
                 this->offset_.y);
}

void Speedometer::update_and_draw(const float kph) const
{
    // If disabled, do nothing
    if (!this->enabled) {
        return;
    }

    // Get the current window size
    const auto [width, height] = this->window_.getSize();

    // Use pivot_.x and pivot_.y to decide how much of the window size to add
    ImGui::SetNextWindowPos({this->pivot_.x * static_cast<float>(width) + this->offset_.x,
                             this->pivot_.y * static_cast<float>(height) + this->offset_.y},
                            ImGuiCond_Always,
                            this->pivot_);  // Corner of the window
    ImGui::Begin("Speedometer",
                 nullptr,
                 base_overlay_flags |
                     ImGuiWindowFlags_AlwaysAutoResize  // Always resize the window to fit its contents
    );

    const int display_kph = static_cast<int>(kph);
    // Cast again to ensure consistency with the "display_kph", then clamp to [0, 1]
    const float speed_fraction = std::clamp(static_cast<float>(display_kph) / this->max_kph_, 0.0f, 1.0f);

    // Display the speed as a progress bar
    const std::string speed_label = std::format("{} kp/h", display_kph);
    ImGui::ProgressBar(speed_fraction,
                       // Calculate the size based on the window size and scale ratio
                       get_scaled_rectangle_size(width, height, this->aspect_ratio_, this->window_scale_ratio_),
                       speed_label.c_str());
    ImGui::End();
}

Minimap::Minimap(sf::RenderTarget &window,
                 const sf::Color &background_color,
                 GameEntitiesDrawer game_entities_drawer,
                 const Corner corner)
    : enabled(true),            // Enable by default
      refresh_interval(0.10f),  // 0.1 second; lower values = more frequent updates but worse performance
      window_(window),
      background_color_(background_color),
      game_entities_drawer_(std::move(game_entities_drawer)),
      pivot_(compute_pivot(corner)),
      offset_(compute_offset(this->pivot_)),
      render_texture_(resolution_)
{
    // Prepare view and texture
    this->view_.setSize(this->capture_size_);  // Set how much of the world to capture (zoom factor, basicallly)
    this->render_texture_.setSmooth(true);     // Enable bilinear filtering for the texture

    SPDLOG_DEBUG("Minimap created at corner '{}', set pivot point to ('{}', '{}') and padding offset to ('{}', '{}') px successfully, exiting constructor!",
                 static_cast<std::underlying_type_t<Corner>>(corner),
                 this->pivot_.x,
                 this->pivot_.y,
                 this->offset_.x,
                 this->offset_.y);
}

void Minimap::update_and_draw(const float dt,
                              const sf::Vector2f &center)
{
    // If disabled, do nothing
    if (!this->enabled) {
        return;
    }

    // The dt is already clamped to [0.001, 0.1] in the main loop, so we can safely use it here

    this->update(dt, center);
    this->draw();
}

void Minimap::update(const float dt,
                     const sf::Vector2f &center)
{
    // Accumulate the delta time
    this->accumulation_ += dt;

    // If the accumulated time exceeds the update rate, refresh the minimap texture
    if (this->accumulation_ >= this->refresh_interval) {
        // Set the view to the center of the minimap
        this->view_.setCenter(center);
        this->render_texture_.setView(this->view_);

        // Draw the game entities onto the render texture
        this->render_texture_.clear(this->background_color_);
        this->game_entities_drawer_(this->render_texture_);
        this->render_texture_.display();

        this->accumulation_ -= this->refresh_interval;  // Keep any overshoot
    }
}

void Minimap::draw() const
{
    // Get the current window size
    const auto [width, height] = this->window_.getSize();

    // Use pivot_.x and pivot_.y to decide how much of the window size to add
    ImGui::SetNextWindowPos({this->pivot_.x * static_cast<float>(width) + this->offset_.x,
                             this->pivot_.y * static_cast<float>(height) + this->offset_.y},
                            ImGuiCond_Always,
                            this->pivot_);  // Corner of the window
    ImGui::Begin("Minimap",
                 nullptr,
                 base_overlay_flags |
                     ImGuiWindowFlags_AlwaysAutoResize  // Always resize the window to fit its contents
    );
    ImGui::Image(this->render_texture_.getTexture(),
                 // Calculate the size based on the window size and scale ratio
                 get_scaled_square_size(width, height, this->window_scale_ratio_));
    ImGui::End();
}

}  // namespace core::ui
