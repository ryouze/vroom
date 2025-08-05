/**
 * @file app.cpp
 */

#include <algorithm>  // for std::clamp, std::max
#include <array>      // for std::array
#include <cstddef>    // for std::size_t
#include <format>     // for std::format
#include <random>     // for std::mt19937, std::random_device
#include <string>     // for std::string
#include <tuple>      // for std::tuple
#include <vector>     // for std::vector

#include <SFML/Graphics.hpp>
// #include <imgui-SFML.h>  // Required for some implicit conversions, e.g., "ImGui::Image(minimap_texture.getTexture(), ...);"
#include <imgui.h>
#include <spdlog/spdlog.h>

#include "app.hpp"
#include "assets/sounds.hpp"
#include "assets/textures.hpp"
#include "core/backend.hpp"
#include "core/colors.hpp"
#include "core/engine.hpp"
#include "core/imgui_sfml_ctx.hpp"
#include "core/input.hpp"
#include "core/io.hpp"
#include "core/states.hpp"
#include "core/widgets.hpp"
#include "core/world.hpp"
#include "game/entities.hpp"
#include "generated.hpp"
#include "settings.hpp"

// Embedded road textures
#include "assets/data/textures/road/road_sand01.hpp"
#include "assets/data/textures/road/road_sand35.hpp"
#include "assets/data/textures/road/road_sand37.hpp"
#include "assets/data/textures/road/road_sand39.hpp"
#include "assets/data/textures/road/road_sand87.hpp"
#include "assets/data/textures/road/road_sand88.hpp"
#include "assets/data/textures/road/road_sand89.hpp"

// Embedded car textures
#include "assets/data/textures/car/car_black_1.hpp"
#include "assets/data/textures/car/car_blue_1.hpp"
#include "assets/data/textures/car/car_green_1.hpp"
#include "assets/data/textures/car/car_red_1.hpp"
#include "assets/data/textures/car/car_yellow_1.hpp"

// Embedded sounds
#include "assets/data/sounds/car/engine.hpp"

namespace app {

void run()
{
    // Define initial game state
    core::states::GameState current_state = core::states::GameState::Menu;

    // Create a RAII context to load and save settings on scope exit
    // This uses platform-specific APIs (e.g., POSIX, WinAPI) to get platform-appropriate paths
    // Then, it loads the configuration from a TOML file, creating default values if the file is missing
    // And it sets values in "settings.hpp"
    // These values can be modified at runtime and on scope exit, the configuration is saved to the TOML file
    core::io::ConfigContext config_context;

    // Create SFML window based on current settings from "settings.hpp"
    // If they were modified by ConfigContext, the window will indeed use these new settings
    core::backend::Window window;

    // Create RAII context with theme and no INI file
    core::imgui_sfml_ctx::ImGuiContext imgui_context{window.raw()};

    // Get window size, update during game loop
    sf::Vector2u window_size_u = window.raw().getSize();
    sf::Vector2f window_size_f = core::backend::to_vector2f(window_size_u);

    // Setup main camera view and zoom factor
    sf::View camera_view;
    // camera_view.setCenter({0.f, 0.f}); // Not needed, because we set it to car position later
    float camera_zoom_factor = 2.5f;
    camera_view.setSize(window_size_f);  // Same as window size
    camera_view.zoom(camera_zoom_factor);

    // Create random number generator
    std::mt19937 rng{std::random_device{}()};

    // Setup texture manager and load textures
    // Note: This cannot be "static", because the destructor for static objects is called after "main()" has finished
    assets::textures::TextureManager textures;
    for (const auto &[identifier, data, size] : {
             // Road textures
             std::tuple{"top_left", road_sand89::data, road_sand89::size},
             std::tuple{"top_right", road_sand01::data, road_sand01::size},
             std::tuple{"bottom_right", road_sand37::data, road_sand37::size},
             std::tuple{"bottom_left", road_sand35::data, road_sand35::size},
             std::tuple{"vertical", road_sand87::data, road_sand87::size},
             std::tuple{"horizontal", road_sand88::data, road_sand88::size},
             std::tuple{"horizontal_finish", road_sand39::data, road_sand39::size},
             // Car textures
             std::tuple{"car_black", car_black_1::data, car_black_1::size},
             std::tuple{"car_blue", car_blue_1::data, car_blue_1::size},
             std::tuple{"car_green", car_green_1::data, car_green_1::size},
             std::tuple{"car_red", car_red_1::data, car_red_1::size},
             std::tuple{"car_yellow", car_yellow_1::data, car_yellow_1::size},
         }) {
        textures.load(identifier, {data, size});
    }

    // Setup sound manager and load sounds
    // Note: This cannot be "static", because the destructor for static objects is called after "main()" has finished
    assets::sounds::SoundManager sounds;
    for (const auto &[identifier, data, size] : {
             // Car sounds
             std::tuple{"engine", engine::data, engine::size},
         }) {
        sounds.load(identifier, {data, size});
    }

    // Create race track
    core::world::Track race_track(
        {.top_left = textures.get("top_left"),
         .top_right = textures.get("top_right"),
         .bottom_right = textures.get("bottom_right"),
         .bottom_left = textures.get("bottom_left"),
         .vertical = textures.get("vertical"),
         .horizontal = textures.get("horizontal"),
         .horizontal_finish = textures.get("horizontal_finish")},
        rng);

    // Create cars
    game::entities::Car player_car(textures.get("car_black"), rng, race_track, game::entities::CarControlMode::Player);
    std::array<game::entities::Car, 4> ai_cars = {
        game::entities::Car(textures.get("car_blue"), rng, race_track, game::entities::CarControlMode::AI),
        game::entities::Car(textures.get("car_green"), rng, race_track, game::entities::CarControlMode::AI),
        game::entities::Car(textures.get("car_red"), rng, race_track, game::entities::CarControlMode::AI),
        game::entities::Car(textures.get("car_yellow"), rng, race_track, game::entities::CarControlMode::AI)};

    // Create gamepad instance
    core::input::Gamepad gamepad(0);

    // Function to reset the cars to their spawn point and reset their speed
    const auto reset_cars = [&player_car, &ai_cars]() {
        // Reset positions of all cars to spawn point
        player_car.reset();
        for (auto &ai_car : ai_cars) {
            ai_car.reset();
        }
    };

    // Shared vehicle names for both leaderboard and combo box
    static constexpr std::array<const char *, 5> vehicle_names = {"Player", "Blue", "Green", "Red", "Yellow"};

    const auto collect_leaderboard_data = [&player_car, &ai_cars]() -> std::vector<core::widgets::LeaderboardEntry> {
        std::vector<core::widgets::LeaderboardEntry> entries;
        entries.reserve(5);  // Reserve space for player + 4 AI cars

        // Add player car
        entries.emplace_back(core::widgets::LeaderboardEntry{vehicle_names[0], player_car.get_state().drift_score, true});

        // Add AI cars using shared names array
        for (std::size_t i = 0; i < ai_cars.size(); ++i) {
            entries.emplace_back(core::widgets::LeaderboardEntry{vehicle_names[i + 1], ai_cars[i].get_state().drift_score, false});
        }

        return entries;
    };

    // Full game reset: restore default track layout, reset cars, reset camera
    const auto reset_game = [&race_track, &reset_cars, &camera_zoom_factor]() {
        race_track.reset();  // Rebuild track with default settings
        reset_cars();
        camera_zoom_factor = 2.5f;
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
            current_state = current_state == core::states::GameState::Playing
                                ? core::states::GameState::Paused
                                : core::states::GameState::Playing;
            break;
        [[unlikely]] case sf::Keyboard::Key::Enter:
            if (current_state == core::states::GameState::Menu)
                current_state = core::states::GameState::Playing;
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

    // List of vehicles and selected index
    const std::array<game::entities::Car *, 5> vehicle_pointer_array = {&player_car, &ai_cars[0], &ai_cars[1], &ai_cars[2], &ai_cars[3]};
    int selected_vehicle_index = 0;

    // Function to draw the game entities (race track and cars) in the window and minimap
    const auto draw_game_entities = [&race_track, &player_car, &ai_cars](sf::RenderTarget &rt) {
        race_track.draw(rt);
        player_car.draw(rt);
        for (const auto &ai_car : ai_cars) {
            ai_car.draw(rt);
        }
    };

    // Build list of fullscreen modes
    std::vector<std::string> mode_names;
    mode_names.reserve(window.available_fullscreen_modes.size());
    for (const auto &mode : window.available_fullscreen_modes) {
        mode_names.emplace_back(std::format("{}x{} ({}-bit)", mode.size.x, mode.size.y, mode.bitsPerPixel));
    }

    // Build C-string array for ImGui
    std::vector<const char *> mode_cstr;
    mode_cstr.reserve(mode_names.size());
    for (auto &name : mode_names) {
        mode_cstr.emplace_back(name.c_str());
    }

    // Widgets
    core::widgets::FpsCounter fps_counter{window.raw()};                                          // FPS counter in the top-left corner
    core::widgets::Minimap minimap{window.raw(), core::colors::window.game, draw_game_entities};  // Minimap in the top-right corner
    core::widgets::Speedometer speedometer{window.raw()};                                         // Speedometer in the bottom-right corner
    core::widgets::Leaderboard leaderboard{window.raw()};                                         // Leaderboard in the top-right corner

    // Engine sound system
    core::engine::EngineSound engine_sound{sounds.get("engine")};

    // TODO: Add vsync and fullscreen saving

    const auto on_event = [&](const sf::Event &event) {
        // Let ImGui handle the event
        imgui_context.process_event(event);

        // Window was closed
        if (event.is<sf::Event::Closed>()) [[unlikely]] {
            window.raw().close();
        }

        // Note: we no longer need this, because we set the view size and zoom on every frame
        // // Window was resized
        // else if (event.is<sf::Event::Resized>()) [[unlikely]] {
        //     // macOS fullscreen fix: query the actual size after resizing
        //     camera_view.setSize(core::backend::to_vector2f(window->getSize()));
        //     camera_view.zoom(camera_zoom_factor);
        // }

        else if (const auto *pressed = event.getIf<sf::Event::KeyPressed>())
            onKeyPressed(*pressed);
        else if (const auto *released = event.getIf<sf::Event::KeyReleased>())
            onKeyReleased(*released);
    };

    const auto on_update = [&](const float dt) {
        imgui_context.update(dt);
        fps_counter.update_and_draw(dt);

        // Get window sizes, highly re-used during game loop and mandatory for correct resizing
        window_size_u = window.raw().getSize();
        window_size_f = core::backend::to_vector2f(window_size_u);

        // Currently selected vehicle
        game::entities::Car *const selected_vehicle_pointer = vehicle_pointer_array[static_cast<std::size_t>(selected_vehicle_index)];

        // Check if gamepad is usable with current configuration
        const bool gamepad_available = gamepad.is_connected();

        if (current_state == core::states::GameState::Playing) [[likely]] {
            game::entities::CarInput player_input = {};
            if (gamepad_available && settings::current::prefer_gamepad) {
                // Use gamepad input
                player_input.throttle = gamepad.get_gas();
                player_input.brake = gamepad.get_brake();
                player_input.steering = gamepad.get_steer();
                player_input.handbrake = gamepad.get_handbrake() ? 1.0f : 0.0f;
            }
            else {
                // Fallback to keyboard state
                // SPDLOG_DEBUG("Controller not connected, using keyboard input!");
                player_input.throttle = key_states.gas ? 1.0f : 0.0f;
                player_input.brake = key_states.brake ? 1.0f : 0.0f;
                player_input.steering = (key_states.left ? -1.0f : 0.0f) + (key_states.right ? 1.0f : 0.0f);
                player_input.handbrake = key_states.handbrake ? 1.0f : 0.0f;
            }
#ifndef NDEBUG  // TODO: Use this debug window to test controller input
            ImGui::Begin("Input");
            ImGui::Text("Controller: %s", gamepad_available ? "Yes" : "No");
            ImGui::Text("Throttle: %.2f", static_cast<double>(player_input.throttle));
            ImGui::Text("Brake: %.2f", static_cast<double>(player_input.brake));
            ImGui::Text("Steering: %.2f", static_cast<double>(player_input.steering));
            ImGui::Text("Handbrake: %.2f", static_cast<double>(player_input.handbrake));
            ImGui::End();
#endif
            player_car.apply_input(player_input);
            player_car.update(dt);
            for (auto &ai : ai_cars) {
                ai.update(dt);
            }
            const auto vehicle_state = selected_vehicle_pointer->get_state();
            camera_view.setCenter(vehicle_state.position);
            camera_view.setSize(window_size_f);
            camera_view.zoom(camera_zoom_factor);
            window.raw().setView(camera_view);
            speedometer.update_and_draw(vehicle_state.speed);
            minimap.update_and_draw(dt, vehicle_state.position);
            leaderboard.update_and_draw(collect_leaderboard_data());

            // Update engine sound based on the currently selected vehicle's speed
            engine_sound.update(vehicle_state.speed);
            if (!engine_sound.is_playing()) {
                engine_sound.start();
            }
        }

        // Paused state, this rarely happens, but more often than the initial menu state, that is gonna be shown only once
        else if (current_state == core::states::GameState::Paused) {
            // Stop engine sound when paused
            engine_sound.stop();

            // Common UI constants
            constexpr float settings_window_width = 500.f;
            constexpr float settings_window_height = 550.f;
            constexpr float button_width = 140.f;
            constexpr float item_width = -200.f;  // Negative width leaves space for labels

            // Since no drawing for the cars and track is done here, only the background color remains
            ImGui::SetNextWindowPos({window_size_f.x * 0.5f, window_size_f.y * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});
            ImGui::SetNextWindowSize({settings_window_width, settings_window_height}, ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
                constexpr float button_count = 3.f;
                const float spacing = ImGui::GetStyle().ItemSpacing.x;
                const float total_width = (button_width * button_count) + (spacing * (button_count - 1.f));
                const float offset = (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;

                if (offset > 0.f) {
                    ImGui::Indent(offset);
                }

                if (ImGui::Button("Resume", {button_width, 0.f})) {
                    current_state = core::states::GameState::Playing;
                }
                ImGui::SameLine();
                if (ImGui::Button("Main Menu", {button_width, 0.f})) {
                    reset_game();
                    current_state = core::states::GameState::Menu;
                }
                ImGui::SameLine();
                if (ImGui::Button("Quit", {button_width, 0.f})) {
                    window.raw().close();
                }

                if (offset > 0.f) {
                    ImGui::Unindent(offset);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::BeginTabBar("settings_tabs")) {
                    if (ImGui::BeginTabItem("Game")) {
                        ImGui::PushItemWidth(item_width);

                        ImGui::SeparatorText("Hacks");
                        if (ImGui::Button("Reset Game")) {
                            reset_game();
                            // Change to playing for instant visual feedback
                            current_state = core::states::GameState::Playing;
                        }

                        bool player_ai_controlled = (player_car.get_state().control_mode == game::entities::CarControlMode::AI);
                        if (ImGui::Checkbox("Enable AI Driver", &player_ai_controlled)) {
                            player_car.set_control_mode(player_ai_controlled ? game::entities::CarControlMode::AI : game::entities::CarControlMode::Player);
                        }

                        ImGui::SeparatorText("Track Layout");
                        const core::world::TrackConfig &track_config = race_track.get_config();
                        int track_width_tiles = static_cast<int>(track_config.horizontal_count);
                        int track_height_tiles = static_cast<int>(track_config.vertical_count);
                        int tile_size_pixels = static_cast<int>(track_config.size_px);
                        float detour_probability = track_config.detour_probability;
                        bool track_config_changed = false;

                        track_config_changed |= ImGui::SliderInt("Width", &track_width_tiles, 3, 30, "%d tiles");
                        track_config_changed |= ImGui::SliderInt("Height", &track_height_tiles, 3, 30, "%d tiles");
                        track_config_changed |= ImGui::SliderInt("Tile Size", &tile_size_pixels, 256, 2048, "%d px");

                        // Technically this isn't a percentage, because we go from 0.f to 1.f, but this code will be removed later, and I don't care
                        if (ImGui::SliderFloat("Detour Probability", &detour_probability, 0.0f, 1.0f, "%.1f")) {
                            detour_probability = std::clamp(detour_probability, 0.0f, 1.0f);
                            track_config_changed = true;
                        }

                        if (track_config_changed) {
                            race_track.set_config({static_cast<std::size_t>(track_width_tiles), static_cast<std::size_t>(track_height_tiles), static_cast<std::size_t>(tile_size_pixels), detour_probability});
                            // Reset all cars to the new track spawn point
                            reset_cars();
                        }

                        ImGui::SeparatorText("Camera");
                        ImGui::SliderFloat("Zoom", &camera_zoom_factor, 1.f, 15.f, "%.1fx");
                        ImGui::Combo("Car", &selected_vehicle_index, vehicle_names.data(), static_cast<int>(vehicle_names.size()));

                        ImGui::PopItemWidth();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Controls")) {
                        ImGui::PushItemWidth(item_width);

                        // Status Overview Section
                        ImGui::SeparatorText("Overview");
                        ImGui::Text("Gamepad Available: %s", gamepad_available ? "Yes" : "No");
                        ImGui::Checkbox("Prefer Gamepad When Available", &settings::current::prefer_gamepad);

                        // Gamepad Configuration Section
                        if (gamepad_available || !settings::current::prefer_gamepad) {
                            ImGui::SeparatorText("Gamepad Configuration");

                            if (!gamepad_available) {
                                ImGui::Text("No gamepad connected - settings will apply when connected");
                            }

                            // Available axes info (compact)
                            if (gamepad_available) {
                                ImGui::TextUnformatted("Available axes:");
                                ImGui::Indent();
                                for (int axis = 0; axis < 8; ++axis) {
                                    if (sf::Joystick::hasAxis(0, static_cast<sf::Joystick::Axis>(axis))) {
                                        if (axis >= 0 && axis < IM_ARRAYSIZE(settings::constants::gamepad_axis_labels)) {
                                            ImGui::BulletText("%s", settings::constants::gamepad_axis_labels[axis]);
                                        }
                                        else {
                                            ImGui::BulletText("Axis %d", axis);
                                        }
                                    }
                                }
                                ImGui::Unindent();
                            }

                            if (ImGui::BeginTable("gamepad_config", 2, ImGuiTableFlags_SizingStretchProp)) {
                                ImGui::TableSetupColumn("Control", ImGuiTableColumnFlags_WidthStretch);
                                ImGui::TableSetupColumn("Binding", ImGuiTableColumnFlags_WidthStretch);

                                // Steering
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextUnformatted("Steering");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Combo("##steering_axis", &settings::current::gamepad_steering_axis, settings::constants::gamepad_axis_labels, IM_ARRAYSIZE(settings::constants::gamepad_axis_labels));
                                ImGui::SameLine();
                                ImGui::Checkbox("Invert##steering", &settings::current::gamepad_invert_steering);

                                // Gas
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextUnformatted("Gas");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Combo("##gas_axis", &settings::current::gamepad_gas_axis, settings::constants::gamepad_axis_labels, IM_ARRAYSIZE(settings::constants::gamepad_axis_labels));
                                ImGui::SameLine();
                                ImGui::Checkbox("Invert##gas", &settings::current::gamepad_invert_gas);

                                // Brake
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextUnformatted("Brake");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Combo("##brake_axis", &settings::current::gamepad_brake_axis, settings::constants::gamepad_axis_labels, IM_ARRAYSIZE(settings::constants::gamepad_axis_labels));
                                ImGui::SameLine();
                                ImGui::Checkbox("Invert##brake", &settings::current::gamepad_invert_brake);

                                // Handbrake
                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::TextUnformatted("Handbrake");
                                ImGui::TableSetColumnIndex(1);
                                ImGui::SliderInt("##handbrake_button", &settings::current::gamepad_handbrake_button, 0, gamepad_available ? static_cast<int>(gamepad.get_button_count()) : 15, "Button %d");

                                ImGui::EndTable();
                            }
                        }

                        if (gamepad_available) {
                            if (ImGui::BeginTable("live_input", 4, ImGuiTableFlags_SizingStretchSame)) {
                                ImGui::TableSetupColumn("Steering");
                                ImGui::TableSetupColumn("Gas");
                                ImGui::TableSetupColumn("Brake");
                                ImGui::TableSetupColumn("Handbrake");
                                ImGui::TableHeadersRow();

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::Text("%.2f", static_cast<double>(gamepad.get_steer()));
                                ImGui::TableSetColumnIndex(1);
                                ImGui::Text("%.2f", static_cast<double>(gamepad.get_gas()));
                                ImGui::TableSetColumnIndex(2);
                                ImGui::Text("%.2f", static_cast<double>(gamepad.get_brake()));
                                ImGui::TableSetColumnIndex(3);
                                ImGui::TextUnformatted(gamepad.get_handbrake() ? "ON" : "OFF");

                                ImGui::EndTable();
                            }
                        }

                        ImGui::SeparatorText("Keyboard Reference");

                        ImGui::BulletText("Accelerate: Up Arrow");
                        ImGui::BulletText("Brake: Down Arrow");
                        ImGui::BulletText("Steer: Left/Right Arrow");
                        ImGui::BulletText("Handbrake: Spacebar");
                        ImGui::BulletText("Pause: ESC");

                        ImGui::PopItemWidth();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Graphics")) {
                        ImGui::PushItemWidth(item_width);

#ifndef NDEBUG
                        ImGui::SeparatorText("Debug");
                        ImGui::BulletText("Resolution: %dx%d", window_size_u.x, window_size_u.y);
#endif

                        ImGui::SeparatorText("Display");
                        if (ImGui::Checkbox("Fullscreen", &settings::current::fullscreen)) {
                            window.recreate();
                        }

                        ImGui::BeginDisabled(!settings::current::fullscreen);
                        if (ImGui::BeginCombo("Resolution", mode_cstr[static_cast<std::size_t>(settings::current::mode_idx)])) {
                            for (int i = 0; i < static_cast<int>(window.available_fullscreen_modes.size()); ++i) {
                                const bool is_selected = (i == settings::current::mode_idx);
                                if (ImGui::Selectable(mode_cstr[static_cast<std::size_t>(i)], is_selected)) {
                                    settings::current::mode_idx = i;
                                    window.recreate();
                                }
                                if (is_selected) {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            ImGui::EndCombo();
                        }
#if defined(__APPLE__)
                        ImGui::TextWrapped("Note: macOS only supports borderless fullscreen mode");
#endif
                        ImGui::EndDisabled();

                        if (ImGui::Combo("Anti-Aliasing", &settings::current::anti_aliasing_idx, settings::constants::anti_aliasing_labels, IM_ARRAYSIZE(settings::constants::anti_aliasing_labels))) {
                            window.recreate();
                        }

                        ImGui::SeparatorText("Frame Rate");
                        if (ImGui::Checkbox("V-Sync", &settings::current::vsync)) {
                            window.recreate();
                            // Hack: set FPS limit's label to "Unlimited", because we don't store previous value
                            settings::current::fps_idx = 8;
                        }

                        ImGui::BeginDisabled(settings::current::vsync);
                        if (ImGui::Combo("FPS Limit", &settings::current::fps_idx, settings::constants::fps_labels, IM_ARRAYSIZE(settings::constants::fps_labels))) {
                            window.recreate();
                        }
                        ImGui::EndDisabled();

                        ImGui::SeparatorText("Widgets");

                        if (ImGui::Checkbox("FPS Counter", &fps_counter.enabled)) {
                        }
                        if (ImGui::Checkbox("Minimap", &minimap.enabled)) {
                        }
                        ImGui::BeginDisabled(!minimap.enabled);
                        ImGui::SliderFloat("Minimap Update Rate", &minimap.refresh_interval, 0.f, 1.f, "%.2f s");

                        // Minimap resolution setting
                        static int minimap_resolution_index = []() {
                            // Initialize to match the default resolution
                            static constexpr sf::Vector2u default_res = {256u, 256u};
                            static constexpr sf::Vector2u resolution_values[] = {{128u, 128u}, {192u, 192u}, {256u, 256u}, {384u, 384u}, {512u, 512u}, {768u, 768u}, {1024u, 1024u}};
                            for (int i = 0; i < static_cast<int>(IM_ARRAYSIZE(resolution_values)); ++i) {
                                if (resolution_values[i] == default_res) {
                                    return i;
                                }
                            }
                            return 2;  // Fallback to 256x256
                        }();
                        static constexpr const char *minimap_resolution_labels[] = {"128x128", "192x192", "256x256", "384x384", "512x512", "768x768", "1024x1024"};
                        static constexpr sf::Vector2u minimap_resolution_values[] = {{128u, 128u}, {192u, 192u}, {256u, 256u}, {384u, 384u}, {512u, 512u}, {768u, 768u}, {1024u, 1024u}};

                        if (ImGui::Combo("Minimap Resolution", &minimap_resolution_index, minimap_resolution_labels, IM_ARRAYSIZE(minimap_resolution_labels))) {
                            minimap.set_resolution(minimap_resolution_values[static_cast<std::size_t>(minimap_resolution_index)]);
                        }
                        ImGui::EndDisabled();
                        if (ImGui::Checkbox("Speedometer", &speedometer.enabled)) {
                        }
                        if (ImGui::Checkbox("Leaderboard", &leaderboard.enabled)) {
                        }

                        ImGui::PopItemWidth();
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("About")) {
                        ImGui::SeparatorText("Build Information");
                        ImGui::BulletText("Version: %s", generated::PROJECT_VERSION);
                        ImGui::BulletText("Build Configuration: %s", generated::BUILD_CONFIGURATION);
                        ImGui::BulletText("Build Date: %s", generated::BUILD_DATE);
                        ImGui::BulletText("Build Time: %s", generated::BUILD_TIME);

                        ImGui::SeparatorText("Compiler Details");
                        ImGui::BulletText("Compiler: %s", generated::COMPILER_INFO);
                        ImGui::BulletText("C++ Standard: %ld", generated::CPP_STANDARD);

                        ImGui::SeparatorText("Build Options");
                        ImGui::BulletText("Build Shared Libs: %s", generated::BUILD_SHARED_LIBS);
                        ImGui::BulletText("Strip Symbols: %s", generated::STRIP_ENABLED);
                        ImGui::BulletText("Link-time Optimization: %s", generated::LTO_ENABLED);

                        ImGui::SeparatorText("Platform");
                        ImGui::BulletText("Operating System: %s (%s)", generated::OPERATING_SYSTEM, generated::ARCHITECTURE);
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }

                ImGui::End();
            }
        }

        // Handle each core::states::GameState
        // Menu state
        else [[unlikely]] {
            // Stop engine sound when in menu
            engine_sound.stop();

            // Main menu
            constexpr float main_menu_width = 240.0f;
            constexpr float button_width = 160.0f;

            // Center the ImGui window inside the SFML window
            ImGui::SetNextWindowPos({window_size_f.x * 0.5f, window_size_f.y * 0.5f}, ImGuiCond_Always, {0.5f, 0.5f});
            ImGui::SetNextWindowSize({main_menu_width, 0.0f}, ImGuiCond_Always);

            if (ImGui::Begin("Main Menu", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar)) {
                const float window_width = ImGui::GetWindowWidth();

                // Title and subtitle
                ImGui::SetCursorPosX((window_width - ImGui::CalcTextSize(generated::PROJECT_NAME).x) * 0.5f);
                ImGui::Text("%s", generated::PROJECT_NAME);
                ImGui::SetCursorPosX((window_width - ImGui::CalcTextSize("2D drift racing game").x) * 0.5f);
                ImGui::TextUnformatted("2D drift racing game");

                ImGui::Separator();

                // Centered buttons
                const float indent_amount = std::max(0.0f, (ImGui::GetContentRegionAvail().x - button_width) * 0.5f);
                ImGui::Indent(indent_amount);

                if (ImGui::Button("Play", {button_width, 0.0f})) {
                    reset_game();
                    current_state = core::states::GameState::Playing;
                }

                if (ImGui::Button("Settings", {button_width, 0.0f})) {
                    current_state = core::states::GameState::Paused;
                }

                if (ImGui::Button("Quit", {button_width, 0.0f})) {
                    window.raw().close();
                }

                ImGui::Unindent(indent_amount);

                ImGui::Separator();

                // Footer
                ImGui::SetCursorPosX((window_width - ImGui::CalcTextSize("Built with C++20 and SFML3").x) * 0.5f);
                ImGui::TextUnformatted("Built with C++20 and SFML3");

                ImGui::SetCursorPosX((window_width - ImGui::CalcTextSize(generated::PROJECT_VERSION).x) * 0.5f);
                ImGui::Text("%s", generated::PROJECT_VERSION);

                ImGui::End();
            }
        }
    };

    const auto on_render = [&](sf::RenderWindow &rt) {
        if (current_state == core::states::GameState::Playing) [[likely]] {
            rt.clear(core::colors::window.game);
            draw_game_entities(rt);
        }
        else if (current_state == core::states::GameState::Paused) {
            rt.clear(core::colors::window.settings);
        }
        else [[unlikely]] {
            rt.clear(core::colors::window.menu);
        }
        imgui_context.render();
        rt.display();
    };

    window.raw().requestFocus();  // Ask OS to switch to this window

    window.run(on_event, on_update, on_render);  // Start the main loop
}

}  // namespace app
