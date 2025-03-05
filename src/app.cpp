/**
 * @file app.cpp
 */

#include <optional>  // for std::optional

#include <SFML/Graphics.hpp>
#include <fmt/core.h>
#include <imgui-SFML.h>
#include <imgui.h>

#include "app.hpp"
#include "core/graphics/debug.hpp"
#include "generated.hpp"

namespace app {

void run()
{
    // Create SFML window with sane defaults
    constexpr sf::Vector2u BASE_WINDOW_SIZE = {800u, 600u};
    auto window = sf::RenderWindow(sf::VideoMode(BASE_WINDOW_SIZE), fmt::format("vroom ({})", PROJECT_VERSION), sf::State::Windowed);
    window.setMinimumSize(window.getSize());  // Never go below the base size
    window.setVerticalSyncEnabled(true);      // Enable VSync to reduce CPU usage
    window.requestFocus();                    // Ask OS to switch to this window

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

    // Create debug corner indicators
    core::graphics::debug::IndicatorRelative indicator_relative(window.getSize());
    const core::graphics::debug::IndicatorAbsolute indicator_absolute(window.getSize());

    // Initialize the main loop
    sf::Clock delta_clock;
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
                indicator_relative.update(new_size);
            }
        }

        // Let ImGui update itself
        ImGui::SFML::Update(window, delta_clock.restart());
        ImGui::ShowDemoWindow();  // DEBUG: Show the demo window

        // Clear with custom color
        window.clear(sf::Color(50, 50, 100));

        // Render the graphics
        indicator_relative.draw(window);
        indicator_absolute.draw(window);
        ImGui::SFML::Render(window);
        window.display();
    }

    // Manually clean up ImGui
    // TODO: Turn this into a RAII class or use a smart pointer
    ImGui::SFML::Shutdown();
}

}  // namespace app
