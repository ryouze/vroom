/**
 * @file widgets.hpp
 *
 * @brief ImGui widgets (speedometer, minimap, FPS counter, etc.) for the game.
 */

#pragma once

#include <cstdint>     // for std::uint32_t
#include <functional>  // for std::function
#include <string>      // for std::string
#include <vector>      // for std::vector

#include <SFML/Graphics.hpp>

namespace core::widgets {

/**
 * @brief Generic enum that represents the corner of the screen, used for positioning.
 */
enum class Corner {
    /**
     * @brief Top-left corner of the screen.
     */
    TopLeft,

    /**
     * @brief Top-right corner of the screen.
     */
    TopRight,

    /**
     * @brief Bottom-left corner of the screen.
     */
    BottomLeft,

    /**
     * @brief Bottom-right corner of the screen.
     */
    BottomRight
};

/**
 * @brief Interface for all UI widgets.
 *
 * All widgets should have an option to enable or disable them. The derived class should implement the "update_and_draw()" method, which will be called once per frame and draw nothing if "enabled" is false.
 */
class IWidget {
  public:
    /**
     * @brief Default destructor.
     */
    virtual ~IWidget() = default;

    /**
     * @brief Enable or disable widget.
     *
     * @note By setting this to false, the "update_and_draw()" method will not perform any calculations or drawing.
     *
     * @details This is enabled by default.
     */
    bool enabled = true;
};

/**
 * @brief Class that computes and displays the current frames per second (FPS) in an ImGui overlay.
 *
 * On construction, the pivot point will be calculated based on the corner provided.
 */
class FpsCounter final : public IWidget {
  public:
    /**
     * @brief Construct a new FpsCounter object.
     *
     * This calculates the pivot point based on the provided corner, but does not perform any FPS calculations until "update_and_draw()" is called.
     *
     * @param window Target window where the FPS counter will be drawn.
     * @param corner Corner of the window where the FPS counter will be displayed (default: "TopLeft").
     */
    explicit FpsCounter(sf::RenderTarget &window,
                        const Corner corner = Corner::TopLeft);

    /**
     * @brief Default destructor.
     */
    ~FpsCounter() = default;

    // Disable copy semantics - holds reference to external resource
    FpsCounter(const FpsCounter &) = delete;
    FpsCounter &operator=(const FpsCounter &) = delete;

    // Allow move construction but disable move assignment (due to reference members)
    FpsCounter(FpsCounter &&) = default;
    FpsCounter &operator=(FpsCounter &&) = delete;

    /**
     * @brief Update the FPS counter and draw it on the provided target as long as "enabled" is true. If "enabled" is false, do nothing.
     *
     * The FPS recalculation is performed only once per second, but the graphics are updated every frame.
     *
     * @param dt Time passed since the previous frame, in seconds.
     *
     * @note Call this once per frame, before ImGui is rendered to the screen (i.e., before "render()").
     */
    void update_and_draw(const float dt);

  private:
    /**
     * @brief Update the FPS measurement logic with the time elapsed since the last frame.
     *
     * The FPS is recalculated only once per second.
     *
     * @param dt Time passed since the previous frame, in seconds.
     *
     * @note This method is called by "update_and_draw()" and is not intended to be called directly. Call this method once each frame. The value of "enabled" does NOT affect this method, use the higher-level "update_and_draw()" method instead.
     */
    void update(const float dt);

    /**
     * @brief Draw a minimal ImGui overlay showing the current FPS in the corner provided during construction.
     *
     * @note This method is called by "update_and_draw()" and is not intended to be called directly. Call this after "update()" and before ImGui is rendered to the screen (i.e., before "render()"). The value of "enabled" does NOT affect this method, use the higher-level "update_and_draw()" method instead.
     */
    void draw() const;

    /**
     * @brief How often to update the FPS counter, in seconds.
     *
     * @note This is the time interval between FPS calculations. The FPS is recalculated only once per second, but the graphics are updated every frame.
     */
    static constexpr float update_rate_ = 1.0f;

    /**
     * @brief Target window where the FPS counter will be drawn.
     */
    const sf::RenderTarget &window_;

    /**
     * @brief Pivot point for the FPS counter window.
     *
     * @note This is basically the corner of the window where the FPS counter will be displayed. The x and y values are in the range "[0, 1]", where "(0, 0)" is the top-left corner and "(1, 1)" is the bottom-right corner.
     */
    ImVec2 pivot_;

    /**
     * @brief Accumulated time since the last FPS calculation.
     */
    float accumulation_;

    /**
     * @brief Number of frames counted within the current accumulation period.
     */
    std::uint32_t frames_;

    /**
     * @brief Most recently computed FPS value.
     */
    std::uint32_t fps_;
};

/**
 * @brief Class that displays the current car speed in an ImGui overlay.
 *
 * On construction, the pivot point and padding offset will be calculated based on the corner provided.
 */
class Speedometer final : public IWidget {
  public:
    /**
     * @brief Construct a new Speedometer object.
     *
     * This calculates the pivot point and padding offset based on the provided corner, but does not perform any drawing until "update_and_draw()" is called.
     *
     * @param window Target window where the speedometer will be drawn.
     * @param corner Corner of the window where the speedometer will be displayed (default: "BottomRight").
     */
    explicit Speedometer(sf::RenderTarget &window,
                         const Corner corner = Corner::BottomRight);

    /**
     * @brief Default destructor.
     */
    ~Speedometer() = default;

    // Disable copy semantics - holds reference to external resource
    Speedometer(const Speedometer &) = delete;
    Speedometer &operator=(const Speedometer &) = delete;

    // Allow move construction but disable move assignment (due to reference members)
    Speedometer(Speedometer &&) = default;
    Speedometer &operator=(Speedometer &&) = delete;

    /**
     * @brief Update the speedometer and draw it on the provided target as long as "enabled" is true. If "enabled" is false, do nothing.
     *
     * @param speed Current car speed in pixels per hour (px/h).
     *
     * @note Call this once per frame, before ImGui is rendered to the screen (i.e., before "render()").
     */
    void update_and_draw(const float speed) const;

  private:
    /**
     * @brief Size of the speedometer window in pixels (width, height).
     */
    static constexpr ImVec2 window_size_ = {200.f, 30.f};

    /**
     * @brief Pixel-to-kilometer per hour conversion factor.
     *
     * @note The original factor was roughly modeled after the Nissan Silvia S14 real-world dimensions ({4.5f, 1.7f}) vs. in-game player car sprite (car_black_1), resulting in "0.1008f". However, that was deemed too high, so it was adjusted to "0.07f" for better scaling.
     */
    static constexpr float px_to_kph_factor_ = 0.07f;

    /**
     * @brief Maximum speed in kilometers per hour, for scaling the progress bar, in pixels.
     */
    static constexpr float max_kph_ = 300.f;

    /**
     * @brief Target window where the speedometer will be drawn.
     */
    const sf::RenderTarget &window_;

    /**
     * @brief Pivot point for the speedometer window.
     *
     * @note This is basically the corner of the window where the speedometer will be displayed. The "x" and "y" values are in the range "[0, 1]", where "(0, 0)" is the top-left corner and "(1, 1)" is the bottom-right corner.
     */
    ImVec2 pivot_;

    /**
     * @brief Padding offset based on the pivot point.
     */
    ImVec2 offset_;
};

/**
 * @brief Class that displays the minimap in an ImGui overlay.
 *
 * The minimap renders the supplied scene into an internal texture at a configurable refresh rate and draws that texture inside an ImGui window.
 */
class Minimap final : public IWidget {
  public:
    /**
     * @brief Function that draws the game entities (e.g., sprites) onto an `sf::RenderTarget`.
     */
    using GameEntitiesDrawer = std::function<void(sf::RenderTarget &)>;

    /**
     * @brief Construct a new Minimap object.
     *
     * This calculates the pivot point and padding offset based on the provided corner but does not render anything until update_and_draw is called.
     *
     * @param window Target window where the minimap will be drawn.
     * @param background_color Background color of the minimap.
     * @param game_entities_drawer Function that draws the game entities (e.g., sprites) onto an `sf::RenderTarget`.
     * @param corner Corner of the window where the minimap will be displayed (default: "BottomLeft").
     */
    explicit Minimap(sf::RenderTarget &window,
                     const sf::Color &background_color,
                     // TODO: Check if the below is true; I'd prefer to use "const" wherever possible.
                     // Cannot use "const" here, because that would copy the callable once when passed in, and again when assigning to the data member. Plus, the const blocks you from moving it.
                     // You can either pass by value and "std::move" it into the member, or use "GameEntitiesDrawer&&"" and std::forward.
                     GameEntitiesDrawer game_entities_drawer,
                     const Corner corner = Corner::BottomLeft);

    /**
     * @brief Default destructor.
     */
    ~Minimap() = default;

    // Disable copy semantics - holds reference to external resource and function object
    Minimap(const Minimap &) = delete;
    Minimap &operator=(const Minimap &) = delete;

    // Allow move construction but disable move assignment (due to reference members)
    Minimap(Minimap &&) = default;
    Minimap &operator=(Minimap &&) = delete;

    /**
     * @brief Update the minimap and draw it on the provided target as long as "enabled" is true. If "enabled" is false, do nothing.
     *
     * @param dt Time passed since the previous frame, in seconds.
     * @param center World-space position to center the minimap on, such as the player's vehicle.
     */
    void update_and_draw(const float dt,
                         const sf::Vector2f &center);

    /**
     * @brief Refresh interval in seconds; values ≤ 0 refresh every frame (default: 0.1 s).
     */
    float refresh_interval;

    /**
     * @brief Set the resolution of the internal render texture.
     *
     * @param new_resolution New resolution for the minimap texture, in pixels.
     *
     * @throws std::runtime_error if the texture resize operation fails.
     */
    void set_resolution(const sf::Vector2u &new_resolution);

    /**
     * @brief Get the current resolution of the internal render texture.
     *
     * @return Current resolution of the minimap texture, in pixels.
     */
    [[nodiscard]] sf::Vector2u get_resolution() const;

  private:
    /**
     * @brief Update the minimap texture.
     *
     * @param dt Time passed since the previous frame, in seconds.
     * @param center World-space position to center the minimap on, such as the player's vehicle.
     *
     * @note This method is called by "update_and_draw()" and is not intended to be called directly. Call this method once each frame. The value of "enabled" does NOT affect this method, use the higher-level "update_and_draw()" method instead.
     */
    void update(const float dt,
                const sf::Vector2f &center);

    /**
     * @brief Draw a minimap in the corner provided during construction.
     *
     * @note This method is called by "update_and_draw()" and is not intended to be called directly. Call this after "update()" and before ImGui is rendered to the screen (i.e., before "render()"). The value of "enabled" does NOT affect this method, use the higher-level "update_and_draw()" method instead.
     */
    void draw() const;

    /**
     * @brief Resolution of the internal render texture, in pixels.
     *
     * @note This is the resolution of the actual minimap texture; we use a low resolution to improve performance.
     */
    sf::Vector2u resolution_;

    /**
     * @brief Default resolution of the internal render texture, in pixels.
     */
    static constexpr sf::Vector2u default_resolution_ = {256u, 256u};

    /**
     * @brief Size of the view used for rendering into the internal texture, in pixels.
     *
     * @note This essentially the size of the world slice that is displayed in the minimap; think of it as a zoom factor, where a higher value means more world area is captured, hence it is more "zoomed out".
     */
    static constexpr sf::Vector2f capture_size_ = {8000.f, -8000.f};

    /**
     * @brief Size of the minimap window in pixels (width, height).
     */
    static constexpr ImVec2 window_size_ = {150.f, 150.f};

    /**
     * @brief Target window where the minimap will be drawn.
     */
    const sf::RenderTarget &window_;

    /**
     * @brief Background color of the minimap.
     */
    const sf::Color &background_color_;

    /**
     * @brief Function that draws the game entities (e.g., sprites) onto an `sf::RenderTarget`.
     *
     * @note This uses type erasure to allow any callable type, such as lambdas or function pointers.
     *
     * @details This cannot be "const", because while it would prevent reassignment (good), it also makes construction harder because you can't move into it without casting away "const". So while the member doesn't need to be mutated, this isn't the right tool for enforcing that.
     */
    GameEntitiesDrawer game_entities_drawer_;

    /**
     * @brief Pivot point for the speedometer window.
     *
     * @note This is basically the corner of the window where the speedometer will be displayed. The "x" and "y" values are in the range "[0, 1]", where "(0, 0)" is the top-left corner and "(1, 1)" is the bottom-right corner.
     */
    ImVec2 pivot_;

    /**
     * @brief Padding offset based on the pivot point.
     */
    ImVec2 offset_;

    /**
     * @brief Internal texture that holds the drawn minimap.
     */
    sf::RenderTexture render_texture_;

    /**
     * @brief View used when rendering into render_texture_.
     */
    sf::View view_;

    /**
     * @brief Accumulated time since the last texture refresh.
     */
    float accumulation_ = 0.f;
};

/**
 * @brief Struct that represents a car's name and drift score for leaderboard display.
 */
struct LeaderboardEntry final {
    /**
     * @brief Car name (e.g., "You", "Blue", "Red", etc.).
     */
    std::string car_name;

    /**
     * @brief Drift score for the car.
     */
    float drift_score;

    /**
     * @brief Whether this entry represents the player (for highlighting).
     */
    bool is_player = false;
};

/**
 * @brief Class that displays the drift leaderboard in an ImGui overlay.
 *
 * Shows the current drift scores for all cars, sorted from highest to lowest score.
 */
class Leaderboard final : public IWidget {
  public:
    /**
     * @brief Construct a new Leaderboard object.
     *
     * This calculates the pivot point and padding offset based on the provided corner, but does not perform any drawing until "update_and_draw()" is called.
     *
     * @param window Target window where the leaderboard will be drawn.
     * @param corner Corner of the window where the leaderboard will be displayed (default: "TopRight").
     */
    explicit Leaderboard(sf::RenderTarget &window,
                         const Corner corner = Corner::TopRight);

    /**
     * @brief Default destructor.
     */
    ~Leaderboard() = default;

    // Disable copy semantics - holds reference to external resource
    Leaderboard(const Leaderboard &) = delete;
    Leaderboard &operator=(const Leaderboard &) = delete;

    // Allow move construction but disable move assignment (due to reference members)
    Leaderboard(Leaderboard &&) = default;
    Leaderboard &operator=(Leaderboard &&) = delete;

    /**
     * @brief Update the leaderboard and draw it on the provided target as long as "enabled" is true. If "enabled" is false, do nothing.
     *
     * The leaderboard data is refreshed at a throttled rate to improve performance, but the graphics are updated every frame.
     *
     * @param dt Time passed since the previous frame, in seconds.
     * @param data_collector Function that collects the current leaderboard data when called.
     *
     * @note Call this once per frame, before ImGui is rendered to the screen (i.e., before "render()").
     */
    void update_and_draw(const float dt,
                         const std::function<std::vector<LeaderboardEntry>()> &data_collector);

  private:
    /**
     * @brief Update the leaderboard data with throttling.
     *
     * @param dt Time passed since the previous frame, in seconds.
     * @param data_collector Function that collects the current leaderboard data when called.
     *
     * @note This method is called by "update_and_draw()" and is not intended to be called directly. Call this method once each frame. The value of "enabled" does NOT affect this method, use the higher-level "update_and_draw()" method instead.
     */
    void update(const float dt,
                const std::function<std::vector<LeaderboardEntry>()> &data_collector);

    /**
     * @brief Draw the leaderboard in the corner provided during construction.
     *
     * @note This method is called by "update_and_draw()" and is not intended to be called directly. Call this after "update()" and before ImGui is rendered to the screen (i.e., before "render()"). The value of "enabled" does NOT affect this method, use the higher-level "update_and_draw()" method instead.
     */
    void draw() const;

    /**
     * @brief How often to update the leaderboard data, in seconds.
     *
     * @note This is set to 20Hz, as higher values (e.g., 60 Hz) are not visually distinguishable, per my own testing.
     */
    static constexpr float update_rate_ = 1.0f / 20.0f;

    /**
     * @brief Size of the leaderboard window  in pixels (width, height).
     */
    static constexpr ImVec2 window_size_ = {250.0f, 160.0f};

    /**
     * @brief Target window where the leaderboard will be drawn.
     */
    const sf::RenderTarget &window_;

    /**
     * @brief Pivot point for the leaderboard window.
     *
     * @note This is basically the corner of the window where the leaderboard will be displayed. The "x" and "y" values are in the range "[0, 1]", where "(0, 0)" is the top-left corner and "(1, 1)" is the bottom-right corner.
     */
    ImVec2 pivot_;

    /**
     * @brief Padding offset based on the pivot point.
     */
    ImVec2 offset_;

    /**
     * @brief Accumulated time since the last leaderboard data update.
     */
    float accumulation_ = 0.0f;

    /**
     * @brief Cached leaderboard entries updated at throttled rate.
     */
    std::vector<LeaderboardEntry> cached_entries_;
};

}  // namespace core::widgets
