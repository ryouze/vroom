/**
 * @file app.cpp
 */

#include <optional>  // for std::optional

#include <SFML/Graphics.hpp>
#include <fmt/core.h>
#include <imgui-SFML.h>
#include <imgui.h>

#include "app.hpp"
#include "generated.hpp"

namespace app {

constexpr sf::Vector2u BASE_WINDOW_SIZE = {1280u, 720u};
constexpr sf::Vector2u MINIMUM_WINDOW_SIZE = BASE_WINDOW_SIZE;
constexpr float FPS_UPDATE_INTERVAL = 1.0f;
constexpr bool DEFAULT_VSYNC_STATE = true;
constexpr float PLAYER_ACCELERATION = 200.f;
constexpr float PLAYER_DECELERATION = 500.f;
constexpr float PLAYER_TURN_SPEED = 100.f;
constexpr float PLAYER_MAX_SPEED = 1500.f;

void run()
{
    // Create SFML window with sane defaults
    auto window = sf::RenderWindow(sf::VideoMode(BASE_WINDOW_SIZE), fmt::format("vroom ({})", PROJECT_VERSION), sf::State::Windowed);
    window.setMinimumSize(MINIMUM_WINDOW_SIZE);          // Never go below the base size
    window.setVerticalSyncEnabled(DEFAULT_VSYNC_STATE);  // Enable VSync to reduce CPU usage
    window.requestFocus();                               // Ask OS to switch to this window
    window.setKeyRepeatEnabled(false);                   // Disable key repeat to avoid multiple events

    // Change the cursor of the window (might be useful when hovering over a button)
    // window.setMouseCursor(sf::Cursor::Type::Hand);

    // Change the position of the window (relatively to the desktop)
    // window.setPosition({10, 50});

    // Change the size of the window
    // window.setSize({640, 480});

    // Change the title of the window
    // window.setTitle("SFML window");

    // Get the size of the window
    // sf::Vector2u size = window.getSize();
    // auto [width, height] = size;

    // Check whether the window has the focus
    // bool focus = window.hasFocus();

    // TODO: RAII or smart pointer
    if (!ImGui::SFML::Init(window)) {  // Load ImGui
        fmt::print("Failed to initialize ImGui-SFML\n");
        return;
    }

    // Player
    sf::RectangleShape player({100.f, 50.f});
    player.setFillColor(sf::Color::Green);
    player.setOrigin(player.getSize() / 2.f);
    player.setPosition({window.getSize().x / 2.f - player.getSize().x / 2.f,
                        window.getSize().y / 2.f - player.getSize().y / 2.f});

    // Initialize the main loop
    sf::Clock clock;

    // FPS tracking
    unsigned frame_count = 0;
    unsigned fps = 0;
    float cumulative_time = 0.0f;
    bool vsync_enabled = DEFAULT_VSYNC_STATE;

    // Movement variables
    float speed = 0.f;
    bool gas = false;
    bool brake = false;
    bool left = false;
    bool right = false;

    sf::Angle angle = sf::degrees(0.f);

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            // Let ImGui handle the event
            ImGui::SFML::ProcessEvent(window, *event);

            // Window was closed
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            // Window was resized
            else if (event->is<sf::Event::Resized>()) {
                // macOS fullscreen fix: query the actual size after resizing
                const sf::Vector2u new_size = window.getSize();
                const sf::FloatRect visible_area({0.f, 0.f}, sf::Vector2f(new_size.x, new_size.y));
                window.setView(sf::View(visible_area));
            }
            // Key pressed
            else if (auto key_event = event->getIf<sf::Event::KeyPressed>()) {
                if (key_event->code == sf::Keyboard::Key::Left) {
                    left = true;
                }
                else if (key_event->code == sf::Keyboard::Key::Right) {
                    right = true;
                }
                else if (key_event->code == sf::Keyboard::Key::Up) {
                    gas = true;
                }
                else if (key_event->code == sf::Keyboard::Key::Down) {
                    brake = true;
                }
            }
            // Key released
            else if (auto key_event2 = event->getIf<sf::Event::KeyReleased>()) {
                if (key_event2->code == sf::Keyboard::Key::Left) {
                    left = false;
                }
                else if (key_event2->code == sf::Keyboard::Key::Right) {
                    right = false;
                }
                else if (key_event2->code == sf::Keyboard::Key::Up) {
                    gas = false;
                }
                else if (key_event2->code == sf::Keyboard::Key::Down) {
                    brake = false;
                }
            }
        }

        // Get delta time
        const float dt = clock.restart().asSeconds();
        cumulative_time += dt;
        ++frame_count;

        // Turn left/right
        if (left) {
            angle -= sf::degrees(PLAYER_TURN_SPEED * dt);
        }
        if (right) {
            angle += sf::degrees(PLAYER_TURN_SPEED * dt);
        }

        // Accelerate or brake
        if (gas) {
            speed += PLAYER_ACCELERATION * dt;
            if (speed > PLAYER_MAX_SPEED) {
                speed = PLAYER_MAX_SPEED;
            }
        }
        if (brake) {
            speed -= PLAYER_DECELERATION * dt;
            if (speed < 0.f) {
                speed = 0.f;  // no reverse for now
            }
        }
        // Natural deceleration (coasting) if not pressing gas/brake
        if (!gas && !brake) {
            speed -= PLAYER_DECELERATION * dt;
            if (speed < 0.f) {
                speed = 0.f;
            }
        }

        // Update player rotation and position
        player.setRotation(angle);

        // For movement, convert angle to radians
        float rad = angle.asRadians();
        float vx = speed * std::cos(rad);
        float vy = speed * std::sin(rad);
        player.move({vx * dt, vy * dt});

        // Prevent the car from leaving the screen
        sf::FloatRect bounds = player.getGlobalBounds();
        float left_edge = bounds.position.x;
        float top_edge = bounds.position.y;
        float width = bounds.size.x;
        float height = bounds.size.y;

        // Keep within left edge
        if (left_edge < 0.f) {
            player.move({-left_edge, 0.f});
        }
        // Keep within top edge
        if (top_edge < 0.f) {
            player.move({0.f, -top_edge});
        }
        // Keep within right edge
        float right_edge = left_edge + width;
        sf::Vector2u window_size = window.getSize();
        if (right_edge > window_size.x) {
            player.move({window_size.x - right_edge, 0.f});
        }
        // Keep within bottom edge
        float bottom_edge = top_edge + height;
        if (bottom_edge > window_size.y) {
            player.move({0.f, window_size.y - bottom_edge});
        }

        // Update ImGui
        ImGui::SFML::Update(window, sf::seconds(dt));

        // Recalculate FPS once per second
        if (cumulative_time >= FPS_UPDATE_INTERVAL) {
            fps = static_cast<unsigned>(frame_count / cumulative_time);
            frame_count = 0;
            cumulative_time = 0.0f;
        }

        // ImGui::ShowDemoWindow();  // DEBUG: Show the demo window
        ImGui::ShowMetricsWindow();

        // Performance overlay (top-left corner)
        constexpr float window_offset = 5.f;
        ImGui::SetNextWindowPos({window_offset, window_offset}, ImGuiCond_Always);
        ImGui::Begin("Debug Overlay", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoMove);
        ImGui::Text("FPS: %d", fps);

        // Retrieve and display the current window size
        ImGui::Text("Size: %dx%d", window_size.x, window_size.y);

        // Toggle vsync with a button click
        if (ImGui::Button(vsync_enabled ? "VSync: ON" : "VSync: OFF")) {
            vsync_enabled = !vsync_enabled;
            window.setVerticalSyncEnabled(vsync_enabled);
        }
        ImGui::End();

        // Clear with custom color
        window.clear(sf::Color(50, 50, 100));

        // Render the graphics
        ImGui::SFML::Render(window);
        window.draw(player);
        window.display();
    }

    // Manually clean up ImGui
    // TODO: Turn this into a RAII class or use a smart pointer
    ImGui::SFML::Shutdown();
}

}  // namespace app
