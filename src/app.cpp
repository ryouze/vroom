/**
 * @file app.cpp
 */

#include <cmath>     // for std::sqrt, std::cos, std::sin
#include <optional>  // for std::optional

#include <SFML/Graphics.hpp>
#include <fmt/core.h>
#include <imgui-SFML.h>
#include <imgui.h>

#include "app.hpp"
#include "generated.hpp"

namespace app {

// Window / UI
constexpr sf::Vector2u BASE_WINDOW_SIZE = {1280u, 720u};
constexpr sf::Vector2u MINIMUM_WINDOW_SIZE = BASE_WINDOW_SIZE;
constexpr float FPS_UPDATE_INTERVAL = 1.0f;
constexpr bool DEFAULT_VSYNC_STATE = true;

// Car parameters
constexpr float CAR_MAX_SPEED = 1500.f;    // clamp max speed
constexpr float CAR_GAS_ACCEL = 400.f;     // pressing Up
constexpr float CAR_BRAKE_ACCEL = 1500.f;  // pressing Down
constexpr float CAR_ENGINE_BRAKE = 400.f;  // friction if coasting
// (Air drag removed)

// Steering
constexpr float STEERING_TURN_RATE = 550.f;        // how fast the wheel turns with left/right
constexpr float STEERING_CENTER_RATE = 450.f;      // how quickly wheel centers if no input
constexpr float STEERING_MAX_ANGLE = 180.f;        // ±180 deg at wheels
constexpr float TURN_FACTOR_AT_FULL_SPEED = 0.8f;  // fraction of steering at max speed
constexpr float TURN_FACTOR_AT_ZERO_SPEED = 1.0f;  // fraction of steering if near 0 speed

void run()
{
    const auto vec_length_fixed = [](const sf::Vector2f &v) -> float {
        return std::sqrt(v.x * v.x + v.y * v.y);
    };

    const auto approach = [](float value, float target, float amount) -> float {
        if (value < target) {
            value += amount;
            if (value > target)
                value = target;
        }
        else if (value > target) {
            value -= amount;
            if (value < target)
                value = target;
        }
        return value;
    };

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

    // Car shape
    sf::RectangleShape car_shape({100.f, 100.f});
    car_shape.setOrigin(car_shape.getSize() / 2.f);
    car_shape.setFillColor(sf::Color::Yellow);
    car_shape.setPosition({static_cast<float>(window.getSize().x) / 2.f,
                           static_cast<float>(window.getSize().y) / 2.f});

    // Load texture
    sf::Texture car_texture;
    if (!car_texture.loadFromFile("/Users/hikari/dev/github-ryouze/!Old SFML Games/vic/assets/game/cars/Export/Player.png")) {
        fmt::print("Failed to load car texture\n");
    }
    else {
        car_shape.setTexture(&car_texture);
    }

    // Track time for each frame
    sf::Clock clock;
    unsigned frame_count = 0;
    unsigned fps = 0;
    float cumulative_time = 0.f;
    bool vsync_enabled = DEFAULT_VSYNC_STATE;

    // Car state
    sf::Vector2f car_velocity(0.f, 0.f);
    sf::Angle car_heading = sf::degrees(0.f);
    float car_steering = 0.f;

    // Key states
    bool gas_pressed = false;
    bool brake_pressed = false;
    // Original behavior: left key sets right steering and right key sets left steering.
    bool turning_right = false;
    bool turning_left = false;

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
            else if (auto key_press = event->getIf<sf::Event::KeyPressed>()) {
                switch (key_press->code) {
                case sf::Keyboard::Key::Up:
                    gas_pressed = true;
                    break;
                case sf::Keyboard::Key::Down:
                    brake_pressed = true;
                    break;
                case sf::Keyboard::Key::Left:
                    turning_left = true;
                    break;
                case sf::Keyboard::Key::Right:
                    turning_right = true;
                    break;
                default:
                    break;
                }
            }
            else if (auto key_release = event->getIf<sf::Event::KeyReleased>()) {
                switch (key_release->code) {
                case sf::Keyboard::Key::Up:
                    gas_pressed = false;
                    break;
                case sf::Keyboard::Key::Down:
                    brake_pressed = false;
                    break;
                case sf::Keyboard::Key::Left:
                    turning_left = false;
                    break;
                case sf::Keyboard::Key::Right:
                    turning_right = false;
                    break;
                default:
                    break;
                }
            }
        }

        // Delta time
        const float dt = clock.restart().asSeconds();
        cumulative_time += dt;
        ++frame_count;

        // 1) Speed & direction
        const float cur_speed = vec_length_fixed(car_velocity);
        float forward_accel = 0.f;
        if (gas_pressed) {
            forward_accel += CAR_GAS_ACCEL;
        }
        if (brake_pressed) {
            forward_accel -= CAR_BRAKE_ACCEL;
        }
        if (forward_accel != 0.f) {
            const float radians = car_heading.asRadians();
            const sf::Vector2f forward_dir(std::cos(radians), std::sin(radians));
            car_velocity += forward_dir * (forward_accel * dt);
        }

        // 2) Engine brake if moving but no gas or brake pressed
        if (!gas_pressed && !brake_pressed && cur_speed > 0.01f) {
            const float decel = CAR_ENGINE_BRAKE * dt;
            float new_speed = cur_speed - decel;
            if (new_speed < 0.f)
                new_speed = 0.f;
            if (cur_speed > 0.f) {
                car_velocity *= (new_speed / cur_speed);
            }
        }

        // Limit top speed
        float spd_now = vec_length_fixed(car_velocity);
        if (spd_now > CAR_MAX_SPEED) {
            car_velocity *= (CAR_MAX_SPEED / spd_now);
            spd_now = CAR_MAX_SPEED;
        }

        // Lateral velocity correction:
        // Align the car's velocity with its heading to prevent sliding/drifting
        {
            const sf::Vector2f forward_dir(std::cos(car_heading.asRadians()),
                                           std::sin(car_heading.asRadians()));
            const float forward_speed = car_velocity.x * forward_dir.x + car_velocity.y * forward_dir.y;
            car_velocity = forward_dir * forward_speed;
        }

        // 4) Steering
        if (turning_right) {
            car_steering += STEERING_TURN_RATE * dt;
        }
        if (turning_left) {
            car_steering -= STEERING_TURN_RATE * dt;
        }
        if (!turning_right && !turning_left) {
            car_steering = approach(car_steering, 0.f, STEERING_CENTER_RATE * dt);
        }
        if (car_steering > STEERING_MAX_ANGLE) {
            car_steering = STEERING_MAX_ANGLE;
        }
        if (car_steering < -STEERING_MAX_ANGLE) {
            car_steering = -STEERING_MAX_ANGLE;
        }
        const float speed_ratio = spd_now / CAR_MAX_SPEED;
        float turn_factor = (TURN_FACTOR_AT_ZERO_SPEED * (1.f - speed_ratio)) +
                            (TURN_FACTOR_AT_FULL_SPEED * speed_ratio);
        if (spd_now < 5.f) {
            turn_factor *= (spd_now / 5.f);
        }
        car_heading += sf::degrees(car_steering * turn_factor * dt);

        // 5) Rotate + move the shape
        car_shape.setRotation(car_heading + sf::degrees(90.f));  // Stupid hack: add +90° to offset texture
        car_shape.move(car_velocity * dt);

        // 6) Bounds check
        const sf::FloatRect b = car_shape.getGlobalBounds();
        const float left_edge = b.position.x;
        const float top_edge = b.position.y;
        const float w = b.size.x;
        const float h = b.size.y;
        const sf::Vector2u win_size = window.getSize();
        const float right_edge = left_edge + w;
        const float bottom_edge = top_edge + h;
        if (left_edge < 0.f) {
            car_shape.move({-left_edge, 0.f});
        }
        if (top_edge < 0.f) {
            car_shape.move({0.f, -top_edge});
        }
        if (right_edge > win_size.x) {
            car_shape.move({win_size.x - right_edge, 0.f});
        }
        if (bottom_edge > win_size.y) {
            car_shape.move({0.f, win_size.y - bottom_edge});
        }

        // 7) Update ImGui + Show FPS
        ImGui::SFML::Update(window, sf::seconds(dt));
        if (cumulative_time >= FPS_UPDATE_INTERVAL) {
            fps = static_cast<unsigned>(frame_count / cumulative_time);
            frame_count = 0;
            cumulative_time = 0.f;
        }
        ImGui::ShowMetricsWindow();
        ImGui::SetNextWindowPos({5.f, 5.f}, ImGuiCond_Always);
        ImGui::Begin("Debug Overlay", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_AlwaysAutoResize |
                         ImGuiWindowFlags_NoMove);
        ImGui::Text("FPS: %u", fps);
        ImGui::Text("Velocity: (%.1f, %.1f)",
                    static_cast<double>(car_velocity.x),
                    static_cast<double>(car_velocity.y));
        ImGui::Text("Speed: %.1f", static_cast<double>(vec_length_fixed(car_velocity)));
        ImGui::Text("Rotation: %.1f deg", static_cast<double>(car_heading.asDegrees()));
        ImGui::Text("Steering: %.1f deg", static_cast<double>(car_steering));
        if (ImGui::Button(vsync_enabled ? "VSync: ON" : "VSync: OFF")) {
            vsync_enabled = !vsync_enabled;
            window.setVerticalSyncEnabled(vsync_enabled);
        }
        ImGui::End();

        // Render frame
        window.clear(sf::Color(50, 50, 100));
        ImGui::SFML::Render(window);
        window.draw(car_shape);
        window.display();
    }

    // Manually clean up ImGui
    // TODO: Turn this into a RAII class or use a smart pointer
    ImGui::SFML::Shutdown();
}

}  // namespace app
