/**
 * @file app.cpp
 */

#include <algorithm>  // for std::clamp, std::max
#include <array>      // for std::array
#include <cstddef>    // for std::size_t
#include <format>     // for std::format
#include <random>     // for std::random_device, std::mt19937
#include <string>     // for std::string. std::to_string
#include <vector>     // for std::vector

#include <SFML/Graphics.hpp>
#include <imgui-SFML.h>  // Required for some implicit conversions, e.g., "ImGui::Image(minimap_texture.getTexture(), ...);"
#include <imgui.h>
#include <spdlog/spdlog.h>

#include "app.hpp"
#include "assets/textures.hpp"  // Embedded textures
#include "core/backend.hpp"
#include "core/game.hpp"
#include "core/io.hpp"
#include "core/misc.hpp"
#include "core/ui.hpp"
#include "generated.hpp"

namespace app {

enum class GameState {
    Playing,
    Paused,
    Menu
};

void run()
{
    // Background colors for each GameState
    constexpr struct {
        sf::Color menu;
        sf::Color game;
        sf::Color settings;
    } window_colors{
        .menu = {210, 180, 140},
        .game = {200, 170, 130},
        .settings = {28, 28, 30},
    };

    // Define initial game state
    GameState current_state = GameState::Menu;

    // Create SFML window with sane defaults and ImGui GUI
    core::backend::Window window;                        // Fullscreen, 144 FPS limit
    core::ui::ImGuiContext imgui_context{window.raw()};  // RAII context with theme and no INI file

    // Get window size, update during game loop
    sf::Vector2u window_size_u = window.get_size();
    sf::Vector2f window_size_f = core::misc::to_vector2f(window_size_u);
    // TODO: Add move "to_vector2f" to backend

    // Create a configuration object to load and save settings
    // This uses platform-specific APIs (e.g., POSIX, WinAPI) to get platform-appropriate paths
    core::io::Config config;

    // Setup main camera view and zoom factor
    sf::View camera_view;
    // camera_view.setCenter({0.f, 0.f}); // Not needed, because we set it to car position later
    float camera_zoom_factor = 2.5f;
    camera_view.setSize(window_size_f);  // Same as window size
    camera_view.zoom(camera_zoom_factor);

    // Load textures, will be automatically managed with RAII, with index access (e.g., "textures[0]")
    // Note: This cannot be "static", because the destructor for static objects is called after "main()" has finished
    const assets::textures::TextureManager road_textures = assets::textures::get_road_textures();
    const assets::textures::TextureManager car_textures = assets::textures::get_car_textures();

    // Create random number generator
    std::mt19937 rng{std::random_device{}()};

    // Build race track
    core::game::Track race_track(
        {.top_left = road_textures[0],
         .top_right = road_textures[1],
         .bottom_right = road_textures[2],
         .bottom_left = road_textures[3],
         .vertical = road_textures[4],
         .horizontal = road_textures[5],
         .horizontal_finish = road_textures[6]},
        rng);

    // AI waypoints (need to be overwritten on reset)
    std::vector<core::game::TrackWaypoint> waypoints = race_track.get_waypoints();

    // Create cars
    core::game::PlayerCar player_car(car_textures[0], rng, race_track);
    std::array<core::game::AICar, 4> ai_cars = {
        core::game::AICar(car_textures[1], rng, race_track),
        core::game::AICar(car_textures[2], rng, race_track),
        core::game::AICar(car_textures[3], rng, race_track),
        core::game::AICar(car_textures[4], rng, race_track)};

    // Function to reset the cars to their spawn point and reset their speed
    const auto reset_cars = [&race_track, &player_car, &ai_cars, &waypoints]() {
        // Reset AI waypoints
        waypoints = race_track.get_waypoints();
        // Reset positions of all cars
        player_car.reset();
        for (auto &ai_car : ai_cars) {
            ai_car.reset();
        }
    };

    // Draw waypoints
    const auto draw_waypoints = [&window, &camera_view, &waypoints]() {
        ImDrawList *foreground_draw_list = ImGui::GetForegroundDrawList();
        for (std::size_t idx = 0; idx < waypoints.size(); ++idx) {
            const core::game::TrackWaypoint &waypoint = waypoints[idx];

            // World to screen
            const sf::Vector2i pixel_position = window.raw().mapCoordsToPixel(waypoint.position, camera_view);
            ImVec2 imgui_position = {static_cast<float>(pixel_position.x), static_cast<float>(pixel_position.y)};
            const std::string label = std::format("Waypoint {}\n({}, {})", idx, waypoint.position.x, waypoint.position.y);
            const ImVec2 text_size = ImGui::CalcTextSize(label.c_str());

            // Center on waypoint
            imgui_position.x -= text_size.x * 0.5f;
            imgui_position.y -= text_size.y * 0.5f;

            foreground_draw_list->AddText(imgui_position, waypoint.type == core::game::TrackWaypoint::DrivingType::Corner ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 0, 255, 255), label.c_str());
        }
    };

    // Player input states
    struct
    {
        bool gas = false, brake = false, left = false, right = false, handbrake = false;
    } key_states;

    // Handle key press events
    const auto onKeyPressed = [&key_states, &current_state](const sf::Event::KeyPressed &pressed) {
        switch (pressed.code) {
        [[likely]] case sf::Keyboard::Key::Up:
            key_states.gas = true;
            break;
        case sf::Keyboard::Key::Down:
            key_states.brake = true;
            break;
        case sf::Keyboard::Key::Left:
            key_states.left = true;
            break;
        case sf::Keyboard::Key::Right:
            key_states.right = true;
            break;
        case sf::Keyboard::Key::Space:
            key_states.handbrake = true;
            break;
        [[unlikely]] case sf::Keyboard::Key::Escape:
            current_state = current_state == GameState::Playing
                                ? GameState::Paused
                                : GameState::Playing;
            break;
        [[unlikely]] case sf::Keyboard::Key::Enter:
            if (current_state == GameState::Menu)
                current_state = GameState::Playing;
            break;
        default:
            break;
        }
    };

    const auto onKeyReleased = [&key_states](const sf::Event::KeyReleased &released) {
        switch (released.code) {
        case sf::Keyboard::Key::Up:
            key_states.gas = false;
            break;
        case sf::Keyboard::Key::Down:
            key_states.brake = false;
            break;
        case sf::Keyboard::Key::Left:
            key_states.left = false;
            break;
        case sf::Keyboard::Key::Right:
            key_states.right = false;
            break;
        [[unlikely]] case sf::Keyboard::Key::Space:
            key_states.handbrake = false;
            break;
        default:
            break;
        }
    };

    // Window settings
    const sf::ContextSettings &window_settings = window.get_settings();

    // List of vehicles
    const std::array<core::game::BaseCar *, 5> vehicle_pointer_array = {&player_car, &ai_cars[0], &ai_cars[1], &ai_cars[2], &ai_cars[3]};

    // Vehicle names
    static constexpr std::array<const char *, 5> vehicle_name_array = {"Player", "AI 1", "AI 2", "AI 3", "AI 4"};
#ifndef NDEBUG  // Debug, remove later
    int selected_vehicle_index = 1;
#else
    int selected_vehicle_index = 0;
#endif

    // Guntime graphics options
    const std::vector<sf::VideoMode> fullscreen_modes = sf::VideoMode::getFullscreenModes();
    int resolution_index = 0;
    bool is_windowed = !window.is_fullscreen();
    bool is_vsync_enabled = window.is_vsync_enabled();

    // FPS limit options
    static constexpr std::array<unsigned, 9> fps_values = {30, 60, 90, 120, 144, 165, 240, 360, 0};
    static constexpr std::array<const char *, 9> fps_labels = {"30", "60", "90", "120", "144", "165", "240", "360", "Unlimited"};
    int fps_index = 4;  // 144
    // TODO: Make the spawned window use the fps index, so we don't need to set it in backend

    // Function to draw the game entities (race track and cars) in the window and minimap
    const auto draw_game_entities = [&race_track, &player_car, &ai_cars](sf::RenderTarget &rt) {
        race_track.draw(rt);
        player_car.draw(rt);
        for (const auto &ai_car : ai_cars) {
            ai_car.draw(rt);
        }
    };

    // Widgets
    core::ui::Minimap minimap{window.raw(), window_colors.game, draw_game_entities};  // Minimap in the top-right corner
    core::ui::FpsCounter fps_counter{window.raw()};                                   // FPS counter in the top-left corner
    core::ui::Speedometer speedometer{window.raw()};                                  // Speedometer in the bottom-right corner

    const auto on_event = [&](const sf::Event &event) {
        // Let ImGui handle the event
        imgui_context.process_event(event);

        // Window was closed
        if (event.is<sf::Event::Closed>()) [[unlikely]] {
            window.close();
        }

        // Note: we no longer need this, because we set the view size and zoom on every frame
        // // Window was resized
        // else if (event.is<sf::Event::Resized>()) [[unlikely]] {
        //     // macOS fullscreen fix: query the actual size after resizing
        //     camera_view.setSize(core::misc::to_vector2f(window->getSize()));
        //     camera_view.zoom(camera_zoom_factor);
        // }

        else if (const auto *pressed = event.getIf<sf::Event::KeyPressed>())
            onKeyPressed(*pressed);
        else if (const auto *released = event.getIf<sf::Event::KeyReleased>())
            onKeyReleased(*released);
    };

    const auto on_update = [&](const float dt) {
        imgui_context.update(dt);
        // TODO: Split "update_and_draw" UI abstractions into separate "update" and "draw" methods so they better fit the current "on_update" "on_render" paradigm
        fps_counter.update_and_draw(dt);

        // Get window sizes, highly re-used during game loop and mandatory for correct resizing
        window_size_u = window.get_size();
        window_size_f = core::misc::to_vector2f(window_size_u);

        // Currently selected vehicle
        core::game::BaseCar *const selected_vehicle_pointer = vehicle_pointer_array[static_cast<std::size_t>(selected_vehicle_index)];

        // Handle each GameState
        // Menu state
        if (current_state == GameState::Menu) [[unlikely]] {
            // Center at screen midpoint, but give a fixed width (200px) and auto-height on first use
            ImGui::SetNextWindowPos({window_size_f.x * 0.5f, window_size_f.y * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});
            ImGui::SetNextWindowSize({200.f, 0.f}, ImGuiCond_FirstUseEver);

            // No AlwaysAutoResize, so width stays at200px
            ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::Spacing();
            ImGui::TextUnformatted(std::format("{} {}", generated::PROJECT_NAME, generated::PROJECT_VERSION).c_str());
            ImGui::Separator();
            ImGui::Spacing();

            // Buttons are centered within the fixed window width
            constexpr float button_width = 150.f;
            const float content_width = ImGui::GetContentRegionAvail().x;
            const float indent = std::max(0.f, (content_width - button_width) * 0.5f);
            ImGui::Indent(indent);
            if (ImGui::Button("Play", {button_width, 0})) {
                current_state = GameState::Playing;
            }
            if (ImGui::Button("Settings", {button_width, 0})) {
                current_state = GameState::Paused;
            }
            if (ImGui::Button("Exit", {button_width, 0})) {
                window.close();
            }
            ImGui::Unindent(indent);
            ImGui::End();
        }

        // Playing state, this is what is gonna happen 99% of the time
        else if (current_state == GameState::Playing) [[likely]] {
            if (key_states.gas) {
                player_car.gas();
            }
            if (key_states.brake) {
                player_car.brake();
            }
            if (key_states.left) {
                player_car.steer_left();
            }
            if (key_states.right) {
                player_car.steer_right();
            }
            if (key_states.handbrake) {
                player_car.handbrake();
            }
            player_car.update(dt);
#ifndef NDEBUG  // Debug, remove later
            ai_cars[0].update(dt);
#else
            for (auto &ai : ai_cars) {
                ai.update(dt);
            }
#endif
            const sf::Vector2f vehicle_position = selected_vehicle_pointer->get_position();
            speedometer.update_and_draw(selected_vehicle_pointer->get_speed());

            camera_view.setCenter(vehicle_position);
            camera_view.setSize(window_size_f);
            camera_view.zoom(camera_zoom_factor);
            window.set_view(camera_view);

            minimap.update_and_draw(dt, vehicle_position);
        }

        // Paused state, this rarely happens, but more often than the initial menu state, that is gonna be shown only once
        else {
            // Since no drawing for the cars and track is done here, only the background color remains
            ImGui::SetNextWindowPos({window_size_f.x * 0.5f, window_size_f.y * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});
            ImGui::SetNextWindowSize({500.f, 550.f}, ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
                ImGui::Spacing();
                {
                    constexpr float resume_button_width = 140.f;
                    constexpr float button_spacing = 10.f;
                    const float available_width = ImGui::GetContentRegionAvail().x;
                    const float buttons_total_width = (resume_button_width * 2.f) + button_spacing;
                    const float indent_amount = (available_width - buttons_total_width) * 0.5f;
                    ImGui::Indent(indent_amount);
                    if (ImGui::Button("Resume", {resume_button_width, 0.f}))
                        current_state = GameState::Playing;
                    ImGui::SameLine(0.f, button_spacing);
                    if (ImGui::Button("Quit to Desktop", {resume_button_width, 0.f}))
                        window.close();
                    ImGui::Unindent(indent_amount);
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                if (ImGui::BeginTabBar("settings_tabs")) {
                    if (ImGui::BeginTabItem("Game")) {
                        ImGui::PushItemWidth(150.f);
                        ImGui::TextUnformatted("Hacks:");
                        if (ImGui::Button("Reset Everything")) {
                            reset_cars();
                            // Change to playing for instant visual feedback
                            current_state = GameState::Playing;
                        }
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        ImGui::TextUnformatted("Track Layout");
                        const core::game::TrackConfig &track_config = race_track.get_config();
                        int track_width_tiles = static_cast<int>(track_config.horizontal_count);
                        int track_height_tiles = static_cast<int>(track_config.vertical_count);
                        int tile_size_pixels = static_cast<int>(track_config.size_px);
                        float detour_probability = track_config.detour_probability;
                        bool track_config_changed = false;
                        track_config_changed |= ImGui::SliderInt("Width", &track_width_tiles, 3, 30, "%d tiles");
                        track_config_changed |= ImGui::SliderInt("Height", &track_height_tiles, 3, 30, "%d tiles");
                        track_config_changed |= ImGui::SliderInt("Tile Size", &tile_size_pixels, 256, 2048, "%d px");
                        // Technicaly this isn't a percentage, because we go from 0.f to 1.f, but this code will be removed later, and I don't care
                        if (ImGui::SliderFloat("Shortcut Chance", &detour_probability, 0.0f, 1.0f, "%.2f")) {
                            detour_probability = std::clamp(detour_probability, 0.0f, 1.0f);
                            track_config_changed = true;
                        }
                        if (track_config_changed) {
                            const core::game::TrackConfig new_config{static_cast<std::size_t>(track_width_tiles), static_cast<std::size_t>(track_height_tiles), static_cast<std::size_t>(tile_size_pixels), detour_probability};  // Rebuild
                            // Reset all cars to the new track spawn point
                            race_track.set_config(new_config);
                            reset_cars();
                        }
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        ImGui::TextUnformatted("Camera");
                        ImGui::SliderFloat("Zoom", &camera_zoom_factor, 1.f, 15.f, "%.2fx");
                        ImGui::Combo("Active Car", &selected_vehicle_index, vehicle_name_array.data(), static_cast<int>(vehicle_name_array.size()));
                        ImGui::PopItemWidth();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Graphics")) {
                        ImGui::PushItemWidth(150.f);
                        ImGui::TextUnformatted("Renderer");
                        ImGui::BulletText("Resolution: %dx%d", window_size_u.x, window_size_u.y);
                        ImGui::BulletText("Anti-Aliasing: %dx", window_settings.antiAliasingLevel);
                        ImGui::BulletText("OpenGL: %d.%d", window_settings.majorVersion, window_settings.minorVersion);
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        ImGui::TextUnformatted("Display Mode");
                        // TODO: Turn this into "Fullscreen" toggle, instead of "Windowed" toggle, to imply that Fullscreen is the default
                        if (ImGui::Checkbox("Windowed", &is_windowed)) {
                            window.set_fullscreen(!is_windowed);
                        }
                        // TODO: This doesn't actualy seem to change the resolution, fix this!
                        ImGui::BeginDisabled(is_windowed);
                        const auto &current_mode = fullscreen_modes[static_cast<std::size_t>(resolution_index)];
                        const std::string resolution_label = std::to_string(current_mode.size.x) + "x" + std::to_string(current_mode.size.y);
                        if (ImGui::BeginCombo("Resolution", resolution_label.c_str())) {
                            for (int i = 0; i < static_cast<int>(fullscreen_modes.size()); ++i) {
                                const auto &mode = fullscreen_modes[static_cast<std::size_t>(i)];
                                std::string label = std::to_string(mode.size.x) + "x" + std::to_string(mode.size.y);
                                if (ImGui::Selectable(label.c_str(), i == resolution_index)) {
                                    resolution_index = i;
                                    if (!is_windowed)
                                        window.set_fullscreen(true, fullscreen_modes[static_cast<std::size_t>(resolution_index)]);
                                }
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::EndDisabled();
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        ImGui::TextUnformatted("Frame Rate");
                        if (ImGui::Checkbox("V-sync", &is_vsync_enabled)) {
                            if (is_vsync_enabled) {
                                window.set_vsync(true);
                            }
                            else {
                                window.set_fps_limit(fps_values[static_cast<std::size_t>(fps_index)]);
                            }
                        }
                        ImGui::BeginDisabled(is_vsync_enabled);
                        if (ImGui::Combo("FPS Limit", &fps_index, fps_labels.data(), static_cast<int>(fps_labels.size())))
                            window.set_fps_limit(fps_values[static_cast<std::size_t>(fps_index)]);
                        ImGui::EndDisabled();
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        ImGui::TextUnformatted("Overlay");
                        ImGui::Checkbox("FPS Counter", &fps_counter.enabled);
                        ImGui::Checkbox("Minimap", &minimap.enabled);
                        ImGui::BeginDisabled(!minimap.enabled);
                        ImGui::SliderFloat("Minimap Refresh", &minimap.refresh_interval, 0.f, 1.f, "%.2fs");
                        ImGui::EndDisabled();
                        ImGui::Checkbox("Speedometer", &speedometer.enabled);
                        ImGui::PopItemWidth();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("System")) {
                        ImGui::BulletText("Version: %s", generated::PROJECT_VERSION);
                        ImGui::BulletText("Build Configuration: %s", generated::BUILD_CONFIGURATION);
                        ImGui::BulletText("Compiler: %s", generated::COMPILER_INFO);
                        ImGui::BulletText("C++ Standard: %ld", generated::CPP_STANDARD);
                        ImGui::BulletText("Build Shared Libs: %s", generated::BUILD_SHARED_LIBS);
                        ImGui::BulletText("Strip Symbols: %s", generated::STRIP_ENABLED);
                        ImGui::BulletText("Link-time Optimization: %s", generated::LTO_ENABLED);
                        ImGui::BulletText("Build Date: %s", generated::BUILD_DATE);
                        ImGui::BulletText("Build Time: %s", generated::BUILD_TIME);
                        ImGui::BulletText("Operating System: %s (%s)", generated::OPERATING_SYSTEM, generated::ARCHITECTURE);
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
                ImGui::End();
            }
        }
    };

    const auto on_render = [&](sf::RenderWindow &rw, [[maybe_unused]] const float dt) {
        if (current_state == GameState::Menu)
            rw.clear(window_colors.menu);
        else if (current_state == GameState::Playing)
            rw.clear(window_colors.game);
        else
            rw.clear(window_colors.settings);

        if (current_state == GameState::Playing) {
            draw_game_entities(rw);
            draw_waypoints();
        }

        imgui_context.render();
        rw.display();
    };

    window.request_focus();  // Ask OS to switch to this window

    window.run(on_event, on_update, on_render);  // Start the main loop
}

}  // namespace app
