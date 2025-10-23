# AGENTS.md

**vroom** is a cross-platform 2D racing game featuring arcade drift physics, procedural track generation, and waypoint-based AI opponents.
It is built to be lightweight, portable, and educational, with a focus on clean C++ architecture and deterministic gameplay behavior.

---

## Technology Stack

| Component                | Description                                |
| ------------------------ | ------------------------------------------ |
| **Language**             | C++20                                      |
| **Graphics**             | SFML 3                                     |
| **UI**                   | Dear ImGui + ImGui-SFML                    |
| **Logging**              | spdlog                                     |
| **Testing**              | snitch                                     |
| **Config**               | toml++                                     |
| **Build System**         | CMake (>= 3.28)                            |
| **Supported OS**         | macOS (Clang), Linux (GCC), Windows (MSVC) |
| **Primary Dev Platform** | macOS 15+                                  |

---

## Repository Layout

```
/.github/        # CI/CD workflows and automation
/assets/         # Screenshots for README.md (irrelevant to AI agents)
/build/          # Out-of-source build directory (ignored in git)
/cmake/          # Extra CMake modules, icons for binaries
/deps/           # External dependencies (CMake FetchContent)
/src/            # Source code root (contains both .hpp and .cpp files)
/src/assets/     # Embedded assets (raw .hpp files) and runtime asset loaders
/src/core/       # Low-level source code
/src/game/       # Gameplay logic
/src/app.cpp     # Application entry point (runs the entire program)
/src/main.cpp    # Main function (sets up basic logging, then calls app.run())
/tests/          # Unit tests (mimics src/ structure, uses snitch)
/CMakeLists.txt  # Root CMake build script (imports modules from cmake/)
```

---

## Build and Run

* Always build from within the `build/` directory using CMake:
  ```sh
  cd build
  cmake ..
  cmake --build . --parallel
  open vroom.app
  ```
* Always run the compiled binary via `open vroom.app` (we are developing on macOS).
* Unit tests are included in the project but are not built by default. You need to pass `-DBUILD_TESTS=ON` to enable them:
  ```sh
  cmake .. -DBUILD_TESTS=ON
  cmake --build . --parallel
  ctest --verbose
  ```
* The `timeout` command is **not available on macOS**.

---

## General Language Guidelines

Applies to **all source files** regardless of language.

* Use **plain ASCII** in logs, code, and comments. No Unicode.
* Use **American English** spelling (`color`, not `colour`).
* Prefer **long, explicit variable names** over abbreviations.
* Never split long lines across newlines. Do not wrap comments.
* Use spaces, not tabs.

---

## C++ Guidelines

* Always access member variables with `this->`.
* Use `snake_case` for functions, variables, and namespaces. Class and struct names must use `CamelCase`. Never use `pascalCase`.
* All member variables in classes end with an underscore (`zoom_ratio_`). Struct fields have no trailing underscore (`zoom_ratio`).
* Use `const`, `constexpr`, and `[[nodiscard]]` wherever possible.
* Do **not** use `noexcept`.
* Always initialize variables with `=` syntax:
  ```cpp
  sf::Vector2u resolution = {1280, 720};
  unsigned fps = 144;
  bool vsync = false;
  ```
* Always format function declarations so the **first parameter stays on the same line** as the function name, with each remaining parameter on its own line:
  ```cpp
  void run(const event_callback_type &on_event,
           const update_callback_type &on_update,
           const render_callback_type &on_render);
  ```
* Every member variable, struct field, and enum constant must include a concise `@brief` Doxygen comment.
* Files in `src/core/` are **mutually independent**.
  They may only depend on the standard library, SFML, or external libs.
  No file in `src/core/` may `#include` another core module.
* Files in `src/game/` may include headers from `src/core/` but **not from other `src/game/` modules**.
  This enforces a one-way dependency from `game` -> `core`.
* The application layer (e.g. `app.cpp`) is allowed to import any `core` or `game` module.

---

### SFML 3 Transition Notes

SFML 3 introduces API changes from SFML 2. Follow these rules:

**1. Vectors, rects, sizes**

* Replace `(x, y)` or `(w, h)` pairs with `sf::Vector2<T>{x, y}`.
* `sf::Rect<T>` now stores `position` and `size` vectors:
  `rect.left` -> `rect.position.x`, `rect.width` -> `rect.size.x`.
* Intersection: `rect.findIntersection(other)` returns `std::optional<sf::Rect<T>>`.

**2. Angles**

* All rotations/angles use `sf::Angle`.
  Construct with `sf::degrees(f)` or `sf::radians(f)`.

**3. Events**

* `pollEvent` and `waitEvent` return `std::optional<sf::Event>`.
  Query using `event->is<sf::Event::Closed>()` or `event->getIf<sf::Event::KeyPressed>()`.

**4. Enumerations and constants**

* All enums are scoped (`sf::Keyboard::Key::A`).
* `sf::Style::Fullscreen` -> `sf::State::Fullscreen`.
* `PrimitiveType::Quads` removed; draw two triangles instead.

**5. Constructors and methods**

| SFML 2                        | SFML 3                 |
| ----------------------------- | ---------------------- |
| `Font::loadFromFile`          | `Font::openFromFile`   |
| `Texture::create(w,h)`        | `Texture::resize(w,h)` |
| `Sound::getLoop/setLoop`      | `isLooping/setLooping` |
| `WindowBase::getSystemHandle` | `getNativeHandle`      |

**6. Types and threading**

* Replace `sf::Int32`/`sf::Uint32` with `<cstdint>` types.
* Replace `sf::Thread`, `sf::Mutex`, `sf::Lock` with standard library equivalents (`std::jthread`, `std::mutex`, `std::lock_guard`).

---

### Logging and Diagnostics

* Logging uses **spdlog**.
  * Debug builds: runtime level = `debug`.
  * Release builds: runtime level = `info`.

---

### Documentation Rules

* All `.hpp` files must contain a Doxygen header block describing the module purpose.
  ```cpp
  /**
   * @file app.hpp
   *
   * @brief Main application logic.
   */
  ```
* The `.cpp` file should only contain the name of the module.
  ```cpp
  /**
   * @file app.cpp
   */
  ```
* Every class, struct, function, and constant requires at least a `@brief` tag.

---

### Communication Style

* I am a non-native English speaker. Use simple English and avoid difficult vocabulary.
* Use strictly formal, technical language only - no slang, no metaphors, no emotional tone, no casual phrasing, no idioms.
