# vroom

[![CI](https://github.com/ryouze/vroom/actions/workflows/ci.yml/badge.svg)](https://github.com/ryouze/vroom/actions/workflows/ci.yml)
[![Release](https://github.com/ryouze/vroom/actions/workflows/release.yml/badge.svg)](https://github.com/ryouze/vroom/actions/workflows/release.yml)
![Release version](https://img.shields.io/github/v/release/ryouze/vroom)

vroom is a cross-platform 2D racing game with arcade drift physics, procedurally-generated tracks, and waypoint AI.

![Screenshot](assets/screenshot.jpeg)
![Settings](assets/settings.jpeg)


## Motivation

I wanted to build a 2D racing game from scratch, without relying on existing game engines like Godot or Unity. To achieve this, I chose to build my own game engine in C++, allowing me to improve my understanding of C++ and game development.

The primary goal is to learn and explore, not to build a groundbreaking game. That said, I still want the final product to be enjoyable for non-developers; I am shipping a playable game, after all.


## Features

- Written in modern C++ (C++20).
- Comprehensive documentation with doxygen-style comments.
- Automatic third-party dependency management using CMake's [FetchContent](https://www.foonathan.net/2022/06/cmake-fetchcontent/).
- No missing STL headers thanks to [header-warden](https://github.com/ryouze/header-warden).
- Single binary distribution with embedded assets thanks to [asset-packer](https://github.com/ryouze/asset-packer).
- Responsive UI and world scaling, with support for ultra-wide resolutions.


## Known Issues

- On macOS, the FPS limiter is unreliable due to timing inaccuracies in SFML; this is unfixable.
  - As a workaround, the default frame rate cap is set to 144 FPS, which seems sufficient for 120 Hz displays found in MBPs.
  - On Linux, it appears to work correctly; moreover, on a Steam Deck OLED, it runs at a consistent 90 FPS while using only 3 watts of power.
  - The game engine can easily exceed 2,500 FPS on Apple M1 Pro systems and 10,000+ FPS on a desktop PC with a dedicated GPU.


## To-Do

```md
**Critical**:
- None.

**Current**:
- Add ImGui window to backend and test again if everything is working correctly.
- Track at which waypoint each car is, regardless of whether it is the player or AI car.
  - OLD NOTE: One option is to store `closest_waypoint_` as a private member in `Car`, updated each frame in `apply_physics_step()`, with a public getter `get_closest_waypoint()`. An alternative is to scan all cars against all waypoints per frame (e.g. in `app.cpp`), which avoids polluting the `Car` class. Use a distance threshold from `Track::get_config()` to determine waypoint proximity.
- Allow AI taking control of the Player via a toggle in the settings.
- Tweak variable names in the `game` module (`.hpp` & `.cpp`).
- Ensure that the minimap's internal resolution either scales automatically with the window size or is configurable through the settings menu. This relates to `Minimap` in `src/core/ui.hpp` and its rendering in `src/app.cpp`.
  - Perhaps we could do both. The minimap is quite expensive to render, so we should definitely allow a lot of customization, in addition to the ability to disable it completely, which is already implemented.
- Implement configuration loading and saving for screen resolution, VSync, and other graphical settings using the `Config` class in `src/core/io.hpp`. The platform-agnostic file path getter is already available; only the logic for loading and saving needs to be implemented.
  - Decide whether to use a custom file format or a ready-made one like TOML or JSON. Rolling our own TOML-like format would likely be the easiest, since we need very few features.
  - Create the directory if it doesn't exist, then store and load settings (e.g., FPS limit, minimap on/off, etc.).
- Check all headers with `header-warden`.
- Add gamepad support.
- Add many different car textures and add more AI cars to the game that use them.
  - The game is very basic, so we can run A LOT of AI cars at once.

**Later**:
- Read the SFML documentation.
  - Study the `sf::View` class in detail to ensure the camera is handled correctly.
- Review and improve `const` usage in UI code, particularly in `src/core/ui.hpp` and `src/core/ui.cpp`. [`src/core/ui.hpp:330`]
- Add keyboard input prompts (game controls on the main menu?). You have already downloaded them from Kenney's website and added to credits.

**Ideas**
- Integrate parallel algorithms. Many STL algorithms (e.g., `copy`, `find`, `sort`) support parallel execution policies such as `seq`, `par`, and `par_unseq`, corresponding to "sequential", "parallel", and "parallel unsequenced", respectively. E.g., `auto result1 = std::find(std::execution::par, std::begin(longVector), std::end(longVector), 2);`.
  - This could be particularly useful for collision checking in `src/core/game.cpp`, which involves iterating over all track tiles until a collision is detected.
- Add `static_assert` checks throughout the codebase (e.g., `static_assert(isIntegral<int>() == true);`) to simplify debugging as the project scales.
  - Also implement compile-time enums and switch-case validation (I forgot why I wanted this?).
- Use the `contains` method for associative containers (e.g., sets, maps) instead of the traditional "find and compare to end" idiom. This applies to areas like texture management in `src/assets/textures.cpp`.
- Use `std::pair` for grouping related values to avoid creating separate variables or structs. Example: `Coordinate = std::pair<int, int>;`.
- Review the debug UI sections in `src/app.cpp` (e.g., lines around 409, 428, 492) and consider if any of this debug information should be exposed in a more polished way or removed.
- Evaluate the necessity of the `#ifndef NDEBUG` blocks for ImGui includes in `src/core/game.cpp` and `src/core/backend.cpp`. Determine if these are still needed or if ImGui should be a standard part of the debug/dev experience.

**Finishing Touches**
- Add basic audio support, with sound effects for the car engine. Avoid adding music, as it would bloat the file size.
- Implement automated testing once the project reaches a mature state.
  - Use a proper testing framework, such as Catch2.
- Improve the packaging workflow:
  - Research the use of CPack for cross-platform distribution.
- Add new screenshots of the main menu, settings, game, and a GIF (or video) of the game in action.
```


## Tested Systems

This project has been tested on the following systems:

- macOS 15.4 (Sequoia)
- Manjaro 24.0 (Wynsdey)
- Windows 11 23H2

Automated testing is also performed on the latest versions of macOS, GNU/Linux, and Windows using GitHub Actions.


## Pre-built Binaries

Pre-built binaries are available for macOS (ARM64), GNU/Linux (x86_64), and Windows (x86_64). You can download the latest version from the [Releases](../../releases) page.

To remove macOS quarantine, use the following commands:

```sh
xattr -d com.apple.quarantine vroom-macos-arm64.app
chmod +x vroom-macos-arm64.app
```

On Windows, the OS might complain about the binary being unsigned. You can bypass this by clicking on "More info" and then "Run anyway".


## Requirements

To build and run this project, you'll need:

- C++20 or higher
- CMake


## Build

Follow these steps to build the project:

1. **Clone the repository**:

    ```sh
    git clone https://github.com/ryouze/vroom.git
    ```

2. **Generate the build system**:

    ```sh
    cd vroom
    mkdir build && cd build
    cmake ..
    ```

    The default configuration (`cmake ..`) is recommended for most users and is also used by CI/CD to build the project.

    However, the build configuration can be customized using the following options:

    - `ENABLE_COMPILE_FLAGS` (default: ON) - Enables strict compiler warnings and treats warnings as errors. When ON, any code warnings will cause compilation to fail, ensuring clean code. When OFF, warnings are allowed and compilation continues. Disable if you encounter compilation issues due to warnings (although CI should catch such issues).
    - `ENABLE_STRIP` (default: ON) - Strips debug symbols from Release builds to reduce binary size. When ON, creates smaller executables but removes debugging information. When OFF, keeps full debugging symbols (useful for debugging crashes). Only affects Release builds.
    - `ENABLE_LTO` (default: ON) - Enables Link Time Optimization for Release builds, producing smaller and faster binaries. When ON, performs cross-module optimizations during linking. When OFF, skips LTO (faster compilation but larger/slower binary). Automatically disabled if compiler doesn't support LTO.
    - `ENABLE_CCACHE` (default: ON) - Optionally uses ccache to cache compilation results for faster rebuilds. When ON and ccache is installed, dramatically speeds up recompilation. When ON but ccache not installed, silently continues without ccache. When OFF, never uses ccache even if available.
    - `BUILD_TESTS` (default: OFF) - Builds unit tests alongside the main executable. When ON, creates test binaries that can be run with `ctest`. When OFF, skips test compilation for faster builds. See [Testing](#testing) for usage.

    Example command to disable strict compile flags and LTO:

    ```sh
    cmake .. -DENABLE_COMPILE_FLAGS=OFF -DENABLE_LTO=OFF
    ```

3. **Compile the project**:

    To compile the project, use the following command:

    ```sh
    cmake --build . --parallel
    ```

After successful compilation, you can run the program using `./vroom` (`open vroom.app` on macOS). However, it is highly recommended to install the program, so that it can be run from any directory. Refer to the [Install](#install) section below.

> [!TIP]
> The mode is set to `Release` by default. To build in `Debug` mode, use `cmake .. -DCMAKE_BUILD_TYPE=Debug`.


## Install

If not already built, follow the steps in the [Build](#build) section and ensure that you are in the `build` directory.

To install the program, use the following command:

```sh
sudo cmake --install .
```

On macOS, this will install the program to `/Applications`. You can then run `vroom.app` from the Launchpad, Spotlight, or by double-clicking the app in Finder.


## Usage

To start the program, simply run the `vroom` executable (`vroom.app` on macOS, `open /Applications/vroom.app` to run from the terminal).


### Controls

- **Gas**: Up Arrow (↑)
- **Handbrake**: Down Arrow (↓)
- **Left**: Left Arrow (←)
- **Right**: Right Arrow (→)
- **Brake**: Spacebar
- **Menu Options**: ESC


## Development

### Debugging

To build with runtime sanitizers and keep debugging symbols, use the following configuration in the `build` directory:

```sh
cmake .. \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DENABLE_STRIP=OFF \
  -DCMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE=OFF \
  -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-fsanitize=address,undefined -fno-omit-frame-pointer" \
  -DCMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO="-fsanitize=address,undefined"
cmake --build . --parallel
```

Then, run the program under `lldb` (macOS):

```sh
lldb ./vroom.app/Contents/MacOS/vroom
run
```

When a sanitizer detects a fault, it will stop execution and print a full stack trace. Use this to pinpoint the root cause of the issue. You can also use `lldb` commands like `bt` (backtrace) to inspect the call stack.


### Logging

The application uses [spdlog](https://github.com/gabime/spdlog) for logging.

For debug builds, the logging level is set to `debug` by default, which is very verbose. For non-debug (Release) builds, the logging level is kept at the default `info` level, which only shows important messages and warnings.

> [!NOTE]
> While `cmake/External.cmake` defines `SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG` in debug builds, this only affects compile-time filtering. The runtime verbosity is still controlled by `spdlog::set_level()`, which must be called to see `debug`-level messages during execution. This is done in `main.cpp`.


### Code Quality Check

To perform a simple quality check on the `src` directory, use the following command from the root of the project:

```sh
./code-check.sh
```

This prints messages for the following issues:
- Missing header guards or `#pragma once` in header files
- TODO/FIXME/HACK comments


### Testing

Tests are included in the project but are not built by default.

To enable and build the tests manually, run the following commands from the `build` directory:

```sh
cmake .. -DBUILD_TESTS=ON
cmake --build . --parallel
ctest --output-on-failure
```


## Credits

**Libraries:**
- [Dear ImGui](https://github.com/ocornut/imgui) - GUI, widgets, overlays, etc.
- [ImGui-SFML](https://github.com/SFML/imgui-sfml) - ImGui-to-SFML binding.
- [Simple and Fast Multimedia Library](https://github.com/sfml/sfml) - Windowing, graphics, input, etc.
- [spdlog](https://github.com/gabime/spdlog) - Logging.

**Graphics:**
- [Input Prompts](https://www.kenney.nl/assets/input-prompts) - Keyboard icons.
- [Moonlight](https://github.com/Madam-Herta/Moonlight) - ImGui theme.
- [PlayCover](https://macosicons.com/#/u/helloman) - App icon.
- [Racing Pack](https://kenney.nl/assets/racing-pack) - Car and track assets.


## Contributing

All contributions are welcome.


## License

This project is licensed under the MIT License.
