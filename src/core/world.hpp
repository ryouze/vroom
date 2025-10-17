/**
 * @file world.hpp
 *
 * @brief Game world abstractions (the race track we drive on).
 */

#pragma once

#include <cstddef>  // for std::size_t
#include <cstdlib>  // for std::abs
#include <random>   // for std::mt19937
#include <vector>   // for std::vector

#include <SFML/Graphics.hpp>

namespace core::world {

/**
 * @brief Struct that represents the configurable parameters of the track. Invalid values will be clamped to reasonable defaults.
 */
struct TrackConfig final {
    /**
     * @brief Number of horizontal tiles, i.e., width (e.g., "8").
     *
     * @note If detours are enabled, +1 tile may be added to each side when a detour occurs, increasing the effective total width, even though the core "horizontal_count" remains unchanged.
     */
    std::size_t horizontal_count = 7;

    /**
     * @brief Number of vertical tiles, i.e., height (e.g., "6").
     *
     * @note This is not affected by detours; they only occur on horizontal edges.
     */
    std::size_t vertical_count = 7;

    /**
     * @brief Size of each tile in pixels (e.g., "256").
     *
     * @note This determines the rendered size of each track tile and does not depend on the source texture size, which will be scaled to the size provided here accordingly.
     *
     * @details The default texture size is 256x256px, so we are scaling it up 6 times.
     */
    std::size_t size_px = 1536;

    /**
     * @brief Probability in the range [0.0, 1.0] that a detour bubble will be generated on each vertical edge segment.
     *
     * @note A value of 0.0 disables detours entirely, while 1.0 maximizes detour frequency. Horizontal edges always remain straight regardless of this setting.
     */
    float detour_probability = 0.7f;

    /**
     * @brief Equality comparison operator with epsilon-based float comparison.
     */
    [[nodiscard]] bool operator==(const TrackConfig &other) const noexcept
    {
        constexpr float epsilon = 1e-6f;
        return this->horizontal_count == other.horizontal_count &&
               this->vertical_count == other.vertical_count &&
               this->size_px == other.size_px &&
               std::abs(this->detour_probability - other.detour_probability) < epsilon;
    }
};

/**
 * @brief Struct that represents a single waypoint on the track, including its position and driving type behavior.
 */
struct TrackWaypoint final {
    /**
     * @brief Enum that represents the driving type of the waypoint.
     *
     * @note This affects how AI behaves when navigating the track, such as whether to slow down for corners or maintain speed on straight segments.
     */
    enum class DrivingType {
        /**
         * @brief Straight-line waypoint; vehicles can maintain full speed.
         */
        Straight,

        /**
         * @brief Corner waypoint; vehicles should slow down, preferably before the corner.
         */
        Corner
    };

    /**
     * @brief World-space coordinates of the waypoint center (e.g., "{100.f, 200.f}").
     */
    sf::Vector2f position;

    /**
     * @brief Driving type of the waypoint (e.g., "Straight" or "Corner").
     */
    DrivingType type;
};

/**
 * @brief Class that manages procedural generation, validation, and rendering of a race track.
 *
 * On construction or configuration change, this class builds the complete track layout from provided tile textures and configuration parameters, which can be drawn to a render target.
 */
class Track final {
  public:
    /**
     * @brief Parameter struct for the track textures. Holds references to the textures used to build the track.
     *
     * The caller is responsible for ensuring that these textures remain valid for the lifetime of the "Track" instance.
     * It is assumed that all textures are square and of the same size (e.g., 256x256) for uniform scaling.
     */
    struct Textures final {
        /**
         * @brief Top-left curve texture.
         *
         * In ASCII:
         * ```
         * XXX
         * X
         * X
         * ```
         */
        const sf::Texture &top_left;

        /**
         * @brief Top-right curve texture.
         *
         * In ASCII:
         * ```
         * XXX
         *   X
         *   X
         * ```
         */
        const sf::Texture &top_right;

        /**
         * @brief Bottom-right curve texture.
         *
         * In ASCII:
         * ```
         *   X
         *   X
         * XXX
         * ```
         */
        const sf::Texture &bottom_right;

        /**
         * @brief Bottom-left curve texture.
         *
         * In ASCII:
         * ```
         * X
         * X
         * XXX
         * ```
         */
        const sf::Texture &bottom_left;

        /**
         * @brief [┃] Vertical road texture.
         *
         * In ASCII:
         * ```
         *  X
         *  X
         *  X
         * ```
         */
        const sf::Texture &vertical;

        /**
         * @brief [━] Horizontal road texture.
         *
         * In ASCII:
         * ```
         *
         * XXX
         *
         * ```
         */
        const sf::Texture &horizontal;

        /**
         * @brief [━] Horizontal finish line texture.
         *
         * In ASCII:
         * ```
         *
         * XXX
         *
         * ```
         */
        const sf::Texture &horizontal_finish;
    };

    /**
     * @brief Construct a new Track object.
     *
     * On construction, the track is automatically built using the provided configuration. The track will be ready for use immediately after construction.
     *
     * @param tiles Tiles struct containing the textures. It is assumed that all textures are square (e.g., 256x256) for uniform scaling. The caller is responsible for ensuring that these textures remain valid for the lifetime of the Track.
     * @param rng Instance of a random number generator (e.g., std::mt19937) used for generating random detours.
     * @param config Configuration struct containing the track configuration (invalid values will be clamped) (default: "TrackConfig()").
     *
     * @details The textures MUST be copied to prevent segfaults.
     */
    explicit Track(const Textures tiles,  //  Copy to prevent segfault
                   std::mt19937 &rng,
                   const TrackConfig &config = TrackConfig());  // Use default config

    /**
     * @brief Get the current validated track configuration.
     *
     * @return Const reference to the current configuration.
     */
    [[nodiscard]] const TrackConfig &get_config() const;

    /**
     * @brief Set the configuration (invalid values will be clamped), then build the track.
     *
     * @param config New configuration for the track; invalid values are clamped during validation.
     */
    void set_config(const TrackConfig &config);

    /**
     * @brief Reset the track to use the default configuration.
     *
     * This rebuilds the track using the default "TrackConfig", effectively restoring it to its initial state.
     */
    void reset();

    /**
     * @brief Check whether a given world-space point lies within any track tile boundary.
     *
     * This performs a simple rectangular collision check against all track tile bounds using pre-cached collision rectangles. While it's technically possible to go outside curved tile shapes visually, this approach is simple and fast.
     *
     * @param world_position Coordinates in world space to test.
     *
     * @return True if the point is inside any tile collision bounds; false otherwise.
     */
    [[nodiscard]] bool is_on_track(const sf::Vector2f &world_position) const;

    /**
     * @brief Get the ordered sequence of waypoints.
     *
     * This is used for AI navigation around the track by following the waypoints sequentially.
     *
     * @return Const reference to the vector of TrackWaypoint instances defining the racing line.
     */
    [[nodiscard]] const std::vector<TrackWaypoint> &get_waypoints() const;

    /**
     * @brief Get the world-space position of the finish line spawn point.
     *
     * This is the position of the finish line tile, used as a spawn point for cars.
     *
     * @return Coordinates of the finish-line tile center.
     */
    [[nodiscard]] const sf::Vector2f &get_finish_position() const;

    /**
     * @brief Draw all track tile sprites onto the provided render target.
     *
     * @param target Target window where the track will be drawn.
     *
     * @note Call this once per frame.
     */
    void draw(sf::RenderTarget &target) const;

    // Disable move semantics
    Track(Track &&) = delete;
    Track &operator=(Track &&) = delete;

    // Disable copy semantics
    Track(const Track &) = delete;
    Track &operator=(const Track &) = delete;

  private:
    /**
     * @brief Return a copy of the configuration with invalid values clamped to safe values.
     *
     * @param config Configuration for the track, possibly containing invalid values.
     *
     * @return Copy of the configuration with invalid values clamped to safe values (e.g., "horizontal_count" and "vertical_count" must be at least "3", "size_px" must be at least "256", and "detour_probability" must be in range [0.0, 1.0]).
     *
     * @note Always use this during construction or configuration changes to prevent catastrophic errors.
     */
    [[nodiscard]] static TrackConfig validate_config(const TrackConfig &config);

    /**
     * @brief Build the track layout using the current configuration and textures.
     *
     * This creates the complete track by placing corner tiles, horizontal/vertical edge tiles, and optional detour bubbles. It also generates collision bounds, waypoint sequences for AI navigation, and identifies the finish line position. Random detour bubbles are inserted on vertical edges according to detour probability, while horizontal edges remain straight.
     *
     * @note This is marked as private, because we only want to build the track on construction and explicit config changes.
     */
    void build();

    /**
     * @brief Tile textures used for building the track.
     *
     * This contains references to all track tile textures including curves, straights, and finish line.
     *
     * @details A complete copy is stored to prevent segfaults if the original texture references become invalid.
     */
    const Textures tiles_;

    /**
     * @brief Random number generator used for procedural track generation.
     *
     * This determines random detour placement along vertical track edges according to the configured detour probability. Each edge segment uses this RNG to decide whether to generate a detour bubble.
     */
    std::mt19937 &rng_;

    /**
     * @brief Current validated track configuration.
     *
     * This contains all track parameters after validation and clamping. Values are guaranteed to be within safe ranges to prevent crashes or invalid track generation.
     */
    TrackConfig config_;

    /**
     * @brief Collection of sprite objects for each tile in the track layout.
     *
     * This contains all track tile sprites positioned and scaled according to the track configuration. Sprites are created during track building and ready for rendering each frame.
     */
    std::vector<sf::Sprite> sprites_;

    /**
     * @brief Axis-aligned bounding rectangles used for collision detection against each sprite.
     *
     * This contains pre-cached collision bounds for efficient track boundary checking. Each rectangle corresponds to a sprite in the "sprites_" vector.
     *
     * @note This ensures that we don't need to call "getGlobalBounds()" on each sprite every time we want to check for collisions.
     */
    std::vector<sf::FloatRect> collision_bounds_;

    /**
     * @brief Ordered sequence of waypoints defining the AI navigation path around the track.
     *
     * This contains waypoints placed at each tile center with appropriate driving type classification (straight or corner). AI cars follow this sequence to navigate around the track efficiently.
     */
    std::vector<TrackWaypoint> waypoints_;

    /**
     * @brief Center position of the finish-line sprite, used as a spawn point for vehicles.
     *
     * This contains the world coordinates of the finish line tile center where cars are initially placed. Set during track building when the finish line tile is positioned on the top edge.
     */
    sf::Vector2f finish_point_ = {0.f, 0.f};
};

}  // namespace core::world
