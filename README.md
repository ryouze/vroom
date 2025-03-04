# vroom

<!-- [![CI](https://github.com/ryouze/vroom/actions/workflows/ci.yml/badge.svg)](https://github.com/ryouze/vroom/actions/workflows/ci.yml)
[![Release](https://github.com/ryouze/vroom/actions/workflows/release.yml/badge.svg)](https://github.com/ryouze/vroom/actions/workflows/release.yml)
![Release version](https://img.shields.io/github/v/release/ryouze/vroom) -->

vroom is a cross-platform 2D racing game.

<!-- ![Screenshot](assets/screenshot.jpeg) -->


## Motivation

I wanted to develop a simple game that would expand my knowledge of SFML, ImGui, and C++ in general. If possible, I'd like it to be playable on my Steam Deck.


### Progress

**Completed:**
- [x] Basic SFML windowing.
- [x] Basic ImGui integration with SFML.
- [x] Basic window resizing.
  - [x] Maximize fix for macOS.

**To-Do:**
- [ ] Robust window resizing (with relative positioning).
  - [ ] Fix resizing with Rectangle on macOS (if possible).
  - [ ] Research SFML's `sf::View` class.
- [ ] Toggle-able fullscreen mode.
  - [ ] Perhaps via ImGui?
- [ ] Platform-specific directories for settings.
  - [ ] Saving of ImGui settings to a platform-specific location instead of relative to the binary (especially on macOS).
- [ ] Basic logging - console & file.
- [ ] Better macOS plist.

**Later:**
- [ ] Basic audio.
- [ ] Automated tests & CI/CD (once the project is more mature).
  - [ ] Use Catch2 or other testing framework instead of reinventing the wheel again.
- [ ] Less hacky setting of titlebar icon.
- [ ] BETTER PACKAGING! Seriously!
  - [ ] Less hacky macOS packaging.
  - [ ] CPack?
  - [ ] .dmg and .exe installers.
- [ ] Gamepad support (maybe? Steam Deck can use Steam Input).


## Features

- Written in modern C++ (C++17).
- Comprehensive documentation with doxygen-style comments.
- Automatic third-party dependency management using CMake's [FetchContent](https://www.foonathan.net/2022/06/cmake-fetchcontent/).
- No missing STL headers thanks to [header-warden](https://github.com/ryouze/header-warden).


## Tested Systems

This project has been tested on the following systems:

- macOS 15.3 (Sequoia)
<!-- - Manjaro 24.0 (Wynsdey)
- Windows 11 23H2 -->

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

- C++17 or higher
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


<!-- ### Controls

text -->


## Testing

Tests are included in the project but are not built by default.

To enable and build the tests manually, run the following commands from the `build` directory:

```sh
cmake .. -DBUILD_TESTS=ON
cmake --build . --parallel
ctest --output-on-failure
```


## Credits

**Libraries:**
- [Dear ImGui](https://github.com/ocornut/imgui)
- [fmt](https://github.com/fmtlib/fmt)
- [ImGui-SFML](https://github.com/SFML/imgui-sfml)
- [Simple and Fast Multimedia Library 3](https://github.com/sfml/sfml)

**Graphics:**
- [PlayCover](https://macosicons.com/#/u/helloman)


## Contributing

All contributions are welcome.


## License

This project is licensed under the MIT License.
