# vroom

[![CI](https://github.com/ryouze/vroom/actions/workflows/ci.yml/badge.svg)](https://github.com/ryouze/vroom/actions/workflows/ci.yml)
[![Release](https://github.com/ryouze/vroom/actions/workflows/release.yml/badge.svg)](https://github.com/ryouze/vroom/actions/workflows/release.yml)
![Release version](https://img.shields.io/github/v/release/ryouze/vroom)

vroom is a cross-platform 2D racing game with arcade physics, procedurally-generated track, and waypoint AI.

![Screenshot](assets/screenshot.jpeg)


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

- The FPS limiter is unreliable due to timing inaccuracies in SFML; this is unfixable.
  - As a workaround, the default frame rate cap is set to 144 FPS, which balances performance and hardware compatibility. The game engine is capable of exceeding 2,500 FPS on Apple M1 Pro systems and 10,000+ FPS on a desktop PC with a dedicated GPU.


## Todo

```md
**Current**:
- Read the SFML documentation.
- Make the minimap's internal resolution either:
  - a) scale automatically with the window size
  - b) be configurable through the settings

**Later**:
- Research the `sf::View` class in detail to ensure correct and flexible camera handling.
- Add config loading/saving via the platform-specific getter function in `core/io.hpp`
- Add basic audio support (e.g., sound effects and background music).
- Add gamepad support if feasible, possibly using Steam Input for Steam Deck compatibility.
- Implement automated tests once the project is mature enough.
  - Use a proper testing framework such as Catch2.
  - Improve packaging process:
  - Simplify macOS packaging, which is currently too hacky.
  - Evaluate the use of CPack for cross-platform packaging.
  - Provide proper `.dmg` and `.exe` installers for distribution.
```


## Tested Systems

This project has been tested on the following systems:

- macOS 15.3 (Sequoia)
- Manjaro 24.0 (Wynsdey)
- Windows 11 23H2

Automated testing is also performed on the latest versions of macOS, GNU/Linux, and Windows using GitHub Actions.


<!-- ## Pre-built Binaries

Pre-built binaries are available for macOS (ARM64), GNU/Linux (x86_64), and Windows (x86_64). You can download the latest version from the [Releases](../../releases) page.

To remove macOS quarantine, use the following commands:

```sh
xattr -d com.apple.quarantine vroom-macos-arm64.app
chmod +x vroom-macos-arm64.app
```

On Windows, the OS might complain about the binary being unsigned. You can bypass this by clicking on "More info" and then "Run anyway". -->


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

    Optionally, you can disable compile warnings by setting `ENABLE_COMPILE_FLAGS` to `OFF`:

    ```sh
    cmake .. -DENABLE_COMPILE_FLAGS=OFF
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

// TODO


## Development

### Logging

The application uses [spdlog](https://github.com/gabime/spdlog) for logging.

For debug builds, the logging level is set to `debug` by default, which is very verbose. For non-debug (Release) builds, the logging level is kept at the default `info` level, which only shows important messages and warnings.

> [!NOTE]
> While `cmake/External.cmake` defines `SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG` in debug builds, this only affects compile-time filtering. The runtime verbosity is still controlled by `spdlog::set_level()`, which must be called to see `debug`-level messages during execution. This is done in `main.cpp`.


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
- [Moonlight](https://github.com/Madam-Herta/Moonlight) - ImGui theme.
- [PlayCover](https://macosicons.com/#/u/helloman) - App icon.
- [Racing Pack](https://kenney.nl/assets/racing-pack) - Car and track assets.


## Contributing

All contributions are welcome.


## License

This project is licensed under the MIT License.
