/**
 * @file app.cpp
 */

#include <algorithm>  // for std::clamp, std::max
#include <array>      // for std::array
#include <cmath>      // for std::hypot
#include <cstddef>    // for std::size_t
#include <format>     // for std::format
#include <memory>     // for std::unique_ptr
#include <optional>   // for std::optional
#include <random>     // for std::random_device, std::mt19937
#include <string>     // for std::string

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
    std::unique_ptr<sf::RenderWindow> window = core::backend::create_window();  // Default: 800p, 144 FPS limit
    core::ui::ImGuiContext imgui_context{*window};                              // RAII context with theme and no INI file

    // Get window size, update during game loop
    sf::Vector2u window_size_u = window->getSize();
    sf::Vector2f window_size_f = core::misc::to_vector2f(window_size_u);

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

    // Initial spawn point
    sf::Vector2f spawn_point = race_track.get_finish_point();

    // Create cars
    constexpr core::game::CarSettings default_car_settings;
    core::game::PlayerCar player_car(car_textures[0], default_car_settings, spawn_point);
    std::array<core::game::AiCar, 4> ai_cars = {
        core::game::AiCar(car_textures[1], default_car_settings, spawn_point),
        core::game::AiCar(car_textures[2], default_car_settings, spawn_point),
        core::game::AiCar(car_textures[3], default_car_settings, spawn_point),
        core::game::AiCar(car_textures[4], default_car_settings, spawn_point)};

    // Function to reset the cars to their spawn point and reset their speed
    const auto reset_cars = [&race_track, &spawn_point, &player_car, &ai_cars]() {
        // Reset positions of all cars
        spawn_point = race_track.get_finish_point();
        player_car.reset(spawn_point);
        for (auto &ai_car : ai_cars) {
            ai_car.reset(spawn_point);
        }
    };

    // Setup AI waypoints
    // std::vector<sf::Vector2f> waypoints = track.build_waypoints();
    // std::array<std::size_t, 4> ai_waypoint_index = {0, 0, 0, 0};

    // Place cars
    // // Lambda for resetting all cars
    // auto reset_all_cars = [&]() {
    //     player.reset();
    //     for (auto &car : ai_cars)
    //         car.reset();
    // };
    // reset_all_cars(player, ai_cars, track);

    // // Let AI update accelerate/brake logic every 0.3s
    // for (AiCar &ai : ai_cars) {
    //     ai.set_update_interval(0.3f);
    // }

    const auto do_collision = [&](core::game::Car &c) {  // TODO: Move this to Car class
        if (!race_track.is_on_track(c.get_position())) {
            c.bounce_back();
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

    // Frame clock and window settings
    sf::Clock clock;
    const sf::ContextSettings &window_settings = window->getSettings();

    // List of vehicles
    const std::array<core::game::Car *, 5> vehicle_pointer_array = {&player_car, &ai_cars[0], &ai_cars[1], &ai_cars[2], &ai_cars[3]};

    // Vehicle names
    static constexpr std::array<const char *, 5> vehicle_name_array = {"Player", "AI 1", "AI 2", "AI 3", "AI 4"};
    int selected_vehicle_index = 0;

    // FPS limit options
    static constexpr std::array<unsigned int, 9> fps_limit_array = {30, 60, 90, 120, 144, 165, 240, 360, 0};
    static constexpr std::array<const char *, 9> fps_option_string_array = {"30", "60", "90", "120", "144", "165", "240", "360", "Unlimited"};

    // Function to draw the game entities (race track and cars) in the window and minimap
    const auto draw_game_entities = [&race_track, &player_car, &ai_cars](sf::RenderTarget &rt) {
        race_track.draw(rt);
        player_car.draw(rt);
        for (const auto &ai_car : ai_cars) {
            ai_car.draw(rt);
        }
    };

    // Widgets
    core::ui::Minimap minimap{*window, window_colors.game, draw_game_entities};  // Minimap in the top-right corner
    core::ui::FpsCounter fps_counter{*window};                                   // FPS counter in the top-left corner
    core::ui::Speedometer speedometer{*window};                                  // Speedometer in the bottom-right corner

    window->requestFocus();  // Ask OS to switch to this window

    // Main loop
    while (window->isOpen()) {
        while (const std::optional event = window->pollEvent()) {
            // Let ImGui handle the event
            imgui_context.process_event(*event);

            // Window was closed
            if (event->is<sf::Event::Closed>()) [[unlikely]] {
                window->close();
            }

            // Note: we no longer need this, because we set the view size and zoom on every frame
            // // Window was resized
            // else if (event->is<sf::Event::Resized>()) [[unlikely]] {
            //     // macOS fullscreen fix: query the actual size after resizing
            //     camera_view.setSize(core::misc::to_vector2f(window->getSize()));
            //     camera_view.zoom(camera_zoom_factor);
            // }

            else if (const auto *pressed = event->getIf<sf::Event::KeyPressed>())
                onKeyPressed(*pressed);
            else if (const auto *released = event->getIf<sf::Event::KeyReleased>())
                onKeyReleased(*released);
        }

        const float delta_time = clock.restart().asSeconds();
        imgui_context.update(delta_time);
        fps_counter.update_and_draw(delta_time);

        // Get window sizes, highly re-used during game loop and mandatory for correct resizing
        window_size_u = window->getSize();
        window_size_f = core::misc::to_vector2f(window_size_u);

        // Currently selected vehicle
        core::game::Car *const selected_vehicle_pointer = vehicle_pointer_array[static_cast<std::size_t>(selected_vehicle_index)];

        // Handle each GameState
        // Menu state
        if (current_state == GameState::Menu) [[unlikely]] {
            window->clear(window_colors.menu);

            // Center at screen midpoint, but give a fixed width (200px) and auto-height on first use
            ImGui::SetNextWindowPos({window_size_f.x * 0.5f, window_size_f.y * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});
            ImGui::SetNextWindowSize({200.0f, 0.0f}, ImGuiCond_FirstUseEver);

            // No AlwaysAutoResize, so width stays at200px
            ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::Spacing();
            {
                const std::string welcome_message = std::format("{} {}", generated::PROJECT_NAME, generated::PROJECT_VERSION);
                ImGui::TextUnformatted(welcome_message.c_str());
            }
            ImGui::Separator();
            ImGui::Spacing();

            // Buttons are centered within the fixed window width
            {
                constexpr float button_width = 150.0f;
                const float content_width = ImGui::GetContentRegionAvail().x;
                const float indent = std::max(0.0f, (content_width - button_width) * 0.5f);
                ImGui::Indent(indent);
                if (ImGui::Button("Play", {button_width, 0.0f})) {
                    current_state = GameState::Playing;
                }
                if (ImGui::Button("Settings", {button_width, 0.0f})) {
                    current_state = GameState::Paused;
                }
                if (ImGui::Button("Exit", {button_width, 0.0f})) {
                    window->close();
                }

                ImGui::Unindent(indent);
            }
            ImGui::Spacing();
            ImGui::End();
        }

        // Playing state, this is what is gonna happen 99% of the time
        else if (current_state == GameState::Playing) [[likely]] {
            player_car.set_input(key_states.gas, key_states.brake, key_states.left, key_states.right, key_states.handbrake);
            player_car.update(delta_time);
            do_collision(player_car);

            for (auto &ai : ai_cars) {
                // ai.update(delta_time);
                do_collision(ai);
            }

            const sf::Vector2f vehicle_position = selected_vehicle_pointer->get_position();
            const sf::Vector2f vehicle_velocity = selected_vehicle_pointer->get_velocity();
            const float speed_kph = core::game::units::px_per_s_to_kph(std::hypot(vehicle_velocity.x, vehicle_velocity.y));
            speedometer.update_and_draw(speed_kph);

            camera_view.setCenter(vehicle_position);
            camera_view.setSize(window_size_f);
            camera_view.zoom(camera_zoom_factor);
            window->setView(camera_view);

            // Clear with game color and draw game objects
            window->clear(window_colors.game);
            draw_game_entities(*window);
            minimap.update_and_draw(delta_time, vehicle_position);
        }

        // Paused state, this rarely happens, but more often than the initial menu state, that is gonna be shown only once
        else {
            window->clear(window_colors.settings);
            // Since no drawing for the cars and track is done here, only the background color remains

            ImGui::SetNextWindowPos({window_size_f.x * 0.5f, window_size_f.y * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});
            ImGui::SetNextWindowSize({500.0f, 550.0f}, ImGuiCond_FirstUseEver);
            ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
            ImGui::Spacing();

            // Horizontal layout: Resume and Exit Game buttons
            {
                constexpr float resume_button_width = 140.0f;
                constexpr float button_spacing = 10.0f;
                const float content_width = ImGui::GetContentRegionAvail().x;
                const float total_buttons_width = (resume_button_width * 2.0f) + button_spacing;
                const float indent = (content_width - total_buttons_width) * 0.5f;
                ImGui::Indent(indent);
                if (ImGui::Button("Play", {resume_button_width, 0.0f})) {
                    current_state = GameState::Playing;
                }
                ImGui::SameLine(0.0f, button_spacing);
                if (ImGui::Button("Exit", {resume_button_width, 0.0f})) {
                    window->close();
                }
                ImGui::Unindent(indent);
            }
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::BeginTabBar("Pause_Dev_Tabs")) {
                if (ImGui::BeginTabItem("Game")) {
                    ImGui::PushItemWidth(150.0f);
                    ImGui::Spacing();
                    ImGui::TextUnformatted("Hacks:");
                    if (ImGui::Button("Reset All")) {
                        reset_cars();
                        // Change to playing for instant visual feedback
                        current_state = GameState::Playing;
                    }
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    ImGui::TextUnformatted("Track:");
                    const core::game::TrackConfig &track_config = race_track.get_config();
                    int track_width_int = static_cast<int>(track_config.horizontal_count);
                    int track_height_int = static_cast<int>(track_config.vertical_count);
                    int tile_size_px = static_cast<int>(track_config.size_px);
                    float detour_chance_float = track_config.detour_chance_pct;
                    bool config_changed = false;
                    if (ImGui::SliderInt("Width (tiles)", &track_width_int, 3, 30)) {
                        config_changed = true;
                    }
                    if (ImGui::SliderInt("Height (tiles)", &track_height_int, 3, 30)) {
                        config_changed = true;
                    }
                    if (ImGui::SliderInt("Size (px)", &tile_size_px, 128, 2048)) {
                        config_changed = true;
                    }
                    if (ImGui::SliderFloat("Detour Chance (%)", &detour_chance_float, 0.0f, 1.0f)) {
                        detour_chance_float = std::clamp(detour_chance_float, 0.0f, 1.0f);
                        config_changed = true;
                    }
                    if (config_changed) {
                        const core::game::TrackConfig new_config{static_cast<std::size_t>(track_width_int), static_cast<std::size_t>(track_height_int), static_cast<std::size_t>(tile_size_px), detour_chance_float};  // Rebuild
                        // Reset all cars to the new track spawn point
                        race_track.set_config(new_config);
                        reset_cars();
                    }
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    ImGui::TextUnformatted("Camera:");
                    ImGui::SliderFloat("Zoom (x)", &camera_zoom_factor, 1.0f, 15.0f, "%.2f");
                    ImGui::PopItemWidth();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Graphics")) {
                    ImGui::PushItemWidth(150.0f);
                    ImGui::Spacing();
                    ImGui::TextUnformatted("Window Info:");
                    ImGui::BulletText("Resolution: %dx%d", window_size_u.x, window_size_u.y);
                    ImGui::BulletText("Anti-Aliasing: %d", window_settings.antiAliasingLevel);
                    ImGui::BulletText("OpenGL Version: %d.%d", window_settings.majorVersion, window_settings.minorVersion);
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    ImGui::TextUnformatted("FPS Limit:");
                    const unsigned int current_fps_limit = core::backend::get_fps_limit();
                    int default_fps_index = 0;
                    for (std::size_t i = 0; i < fps_limit_array.size(); ++i) {
                        if (fps_limit_array[i] == current_fps_limit) {
                            default_fps_index = static_cast<int>(i);
                            break;
                        }
                    }
                    int selected_fps_index = default_fps_index;
                    if (ImGui::Combo("##fps_limit", &selected_fps_index, fps_option_string_array.data(), static_cast<int>(fps_option_string_array.size()))) {
                        core::backend::set_fps_limit(*window, fps_limit_array[static_cast<std::size_t>(selected_fps_index)]);
                    }
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    ImGui::TextUnformatted("Toggles:");
                    ImGui::Checkbox("Show Minimap", &minimap.enabled);
                    ImGui::Checkbox("Show Speedometer", &speedometer.enabled);
                    ImGui::Checkbox("Show FPS Counter", &fps_counter.enabled);
                    ImGui::Spacing();
                    ImGui::TextUnformatted("Minimap:");
                    ImGui::SliderFloat(" Refresh Rate (s)", &minimap.refresh_interval, 0.0f, 1.0f, "%.2f");
                    ImGui::PopItemWidth();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Cars")) {
                    ImGui::PushItemWidth(150.0f);
                    ImGui::TextUnformatted("Select Car:");
                    ImGui::Combo("##car", &selected_vehicle_index, vehicle_name_array.data(), static_cast<int>(vehicle_name_array.size()));
                    ImGui::Spacing();
                    ImGui::TextUnformatted("Car Info:");
                    const sf::Vector2f position_for_display = selected_vehicle_pointer->get_position();
                    const sf::Vector2f velocity_for_display = selected_vehicle_pointer->get_velocity();
                    ImGui::BulletText(
                        "Position: (%.1f, %.1f)",
                        static_cast<double>(position_for_display.x),
                        static_cast<double>(position_for_display.y));
                    ImGui::BulletText(
                        "Velocity: (%.1f, %.1f)",
                        static_cast<double>(velocity_for_display.x),
                        static_cast<double>(velocity_for_display.y));
                    ImGui::PopItemWidth();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("System")) {
                    ImGui::Spacing();
                    ImGui::TextUnformatted("Build Info:");
                    ImGui::BulletText("Project Version: %s", generated::PROJECT_VERSION);
                    ImGui::BulletText("Build Configuration: %s", generated::BUILD_CONFIGURATION);
                    ImGui::BulletText("Compiler: %s", generated::COMPILER_INFO);
                    ImGui::BulletText("C++ Standard: %ld", generated::CPP_STANDARD);
                    ImGui::BulletText("Build Shared Libs: %s", generated::BUILD_SHARED_LIBS);
                    ImGui::BulletText("Strip Enabled: %s", generated::STRIP_ENABLED);
                    ImGui::BulletText("LTO Enabled: %s", generated::LTO_ENABLED);
                    ImGui::BulletText("Build Date: %s", generated::BUILD_DATE);
                    ImGui::BulletText("Build Time: %s", generated::BUILD_TIME);
                    ImGui::BulletText("Operating System: %s (%s)",
                                      generated::OPERATING_SYSTEM,
                                      generated::ARCHITECTURE);
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }

            ImGui::End();
        }

        imgui_context.render();
        window->display();
    }
}

}  // namespace app
