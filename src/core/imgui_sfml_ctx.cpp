/**
 * @file imgui_sfml_ctx.cpp
 */

#include <stdexcept>  // for std::runtime_error

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include "imgui_sfml_ctx.hpp"

namespace core::imgui_sfml_ctx {

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

}  // namespace core::imgui_sfml_ctx
