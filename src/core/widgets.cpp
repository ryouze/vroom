/**
 * @file widgets.cpp
 */

#include <algorithm>    // for std::clamp, std::sort
#include <cstddef>      // for std::size_t
#include <cstdint>      // for std::uint32_t
#include <format>       // for std::format
#include <functional>   // for std::function
#include <stdexcept>    // for std::runtime_error
#include <string>       // for std::string
#include <type_traits>  // for std::underlying_type_t
#include <utility>      // for std::move
#include <vector>       // for std::vector

#include <imgui-SFML.h>
#include <imgui.h>
#include <SFML/Graphics/Color.hpp>
#include <spdlog/spdlog.h>

#include "widgets.hpp"

namespace core::widgets {

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

}  // namespace

FpsCounter::FpsCounter(sf::RenderTarget &window,
                       const Corner corner)
    : window_(window),
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
    : window_(window),
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

void Speedometer::update_and_draw(const float speed) const
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

    // Convert speed from px/h to kph, casting to int to get rid of decimals
    const int display_kph = static_cast<int>(speed * this->px_to_kph_factor_);
    // Display the speed as a progress bar
    ImGui::ProgressBar(
        // Cast again to ensure consistency with the "display_kph", then clamp to [0, 1]
        std::clamp(static_cast<float>(display_kph) / this->max_kph_, 0.0f, 1.0f),
        this->window_size_, std::format("{} kp/h", display_kph).c_str());
    ImGui::End();
}

Minimap::Minimap(sf::RenderTarget &window,
                 const sf::Color &background_color,
                 GameEntitiesDrawer game_entities_drawer,
                 const Corner corner)
    : refresh_interval(0.1f),  // 0.1 second; lower values = more frequent updates but worse performance
      resolution_(default_resolution_),
      window_(window),
      background_color_(background_color),
      game_entities_drawer_(std::move(game_entities_drawer)),
      pivot_(compute_pivot(corner)),
      offset_(compute_offset(this->pivot_)),
      render_texture_(this->resolution_)
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
                 this->window_size_);
    ImGui::End();
}

void Minimap::set_resolution(const sf::Vector2u &new_resolution)
{
    SPDLOG_DEBUG("Setting minimap resolution from ('{}', '{}') to ('{}', '{}')", this->resolution_.x, this->resolution_.y, new_resolution.x, new_resolution.y);
    this->resolution_ = new_resolution;
    if (!this->render_texture_.resize(this->resolution_)) [[unlikely]] {
        throw std::runtime_error(std::format("Failed to resize minimap render texture to ('{}', '{}')",
                                             this->resolution_.x,
                                             this->resolution_.y));
    }
    // Maintain smoothing after resize
    this->render_texture_.setSmooth(true);
    SPDLOG_DEBUG("Minimap resolution changed successfully to ('{}', '{}')", this->resolution_.x, this->resolution_.y);
}

sf::Vector2u Minimap::get_resolution() const
{
    return this->resolution_;
}

Leaderboard::Leaderboard(sf::RenderTarget &window,
                         const Corner corner)
    : window_(window),
      pivot_(compute_pivot(corner)),
      offset_(compute_offset(this->pivot_))
{
    SPDLOG_DEBUG("Leaderboard created at corner '{}', set pivot point to ('{}', '{}') and padding offset to ('{}', '{}') px successfully, exiting constructor!",
                 static_cast<std::underlying_type_t<Corner>>(corner),
                 this->pivot_.x,
                 this->pivot_.y,
                 this->offset_.x,
                 this->offset_.y);
}

void Leaderboard::update_and_draw(const float dt,
                                  const std::function<std::vector<LeaderboardEntry>()> &data_collector)
{
    // If disabled, do nothing
    if (!this->enabled) {
        return;
    }

    this->update(dt, data_collector);
    this->draw();
}

void Leaderboard::update(const float dt,
                         const std::function<std::vector<LeaderboardEntry>()> &data_collector)
{
    // Accumulate the delta time
    this->accumulation_ += dt;

    // If the accumulated time exceeds the update rate, refresh the leaderboard data
    if (this->accumulation_ >= this->update_rate_) {
        // Collect fresh data only when throttle interval is reached
        this->cached_entries_ = data_collector();

        // Sort the cached entries for display (highest score first)
        std::sort(this->cached_entries_.begin(), this->cached_entries_.end(),
                  [](const LeaderboardEntry &a, const LeaderboardEntry &b) {
                      return a.drift_score > b.drift_score;  // Descending order
                  });

        this->accumulation_ -= this->update_rate_;  // Keep any overshoot
    }
}

void Leaderboard::draw() const
{
    // Get the current window size
    const auto [width, height] = this->window_.getSize();

    // Use pivot_.x and pivot_.y to decide how much of the window size to add
    ImGui::SetNextWindowPos({this->pivot_.x * static_cast<float>(width) + this->offset_.x,
                             this->pivot_.y * static_cast<float>(height) + this->offset_.y},
                            ImGuiCond_Always,
                            this->pivot_);  // Corner of the window
    ImGui::SetNextWindowSize(this->window_size_, ImGuiCond_Always);

    ImGui::Begin("Drift Scores",
                 nullptr,
                 base_overlay_flags |
                     ImGuiWindowFlags_NoResize  // Prevent manual resizing
    );

    // Display header
    ImGui::Text("Drift Scores");
    ImGui::Separator();

    // Display cached entries with position numbers
    for (std::size_t i = 0; i < this->cached_entries_.size(); ++i) {
        const auto &entry = this->cached_entries_[i];

        // Set yellow color for player entries
        if (entry.is_player) {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));  // Yellow
        }

        // Format score as integer (no decimals)
        // const int display_score = static_cast<int>(entry.drift_score);
        const std::string display_score = std::format("{:.0f} pts", entry.drift_score);
        ImGui::Text("%zu. %s: %s", i + 1, entry.car_name.c_str(), display_score.c_str());

        // Reset color for player entries
        if (entry.is_player) {
            ImGui::PopStyleColor();
        }
    }

    // If no entries, show placeholder
    if (this->cached_entries_.empty()) {
        ImGui::Text("No cars detected");
    }

    ImGui::End();
}

}  // namespace core::widgets
