This is `vroom`, a cross-platform 2D racing game with arcade drift physics, procedurally-generated tracks, and waypoint AI.

## Technology Stack
- Language: C++20.
- Graphics: SFML3.
- UI: Dear ImGui.
- Logging: spdlog.
- Target Platforms (in order of importance): macOS, Linux, Windows.
- Development Environment: macOS/Clang/CMake.

## All Languages
- Use plain ASCII in logs and comments; avoid Unicode.
- Use American English spelling (e.g., `color`).
- Prefer long, verbose variable names.
- Do not split long lines across newlines, keep them long - even if it hurts readability. I especially hate when long comments are split across multiple lines.

## C++
- Always prefix member variables with `this->`.
- Use `snake_case` for all identifiers, except for class and struct names, which must use `CamelCase`. Avoid `pascalCase` at all costs.
- Use `const`, `constexpr`, and `[[nodiscard]]` whenever possible.
- Class member variables must have a trailing underscore (e.g., `zoom_ratio_`), while struct fields should not (e.g., `zoom_ratio`).
- Use CMake from the `build` directory, e.g., `cmake --build . --parallel`.
- All files in `src/core/` (e.g., `game.hpp`, `backend.hpp`, `ui.hpp`) must be completely independent - they cannot `#include` other core modules. Each core file should only depend on standard library, SFML, and external libraries. This creates a "flat" architecture where higher-level code (like `app.cpp`) can freely import any core modules without circular dependency issues. Think of core modules as independent building blocks that can be mixed and matched by the application layer.
- Do not use `noexcept`.
- Use modern C++20 features and SFML3. SFML3 uses C++17 and has the following differences from SFML2:
```md
### 1. Vectors, rects, sizes
* Replace any `(x, y)` or `(w, h)` parameter pair with `sf::Vector2<T>{x, y}`.
  * Example `circle.setPosition({10, 20});`
* `sf::Rect<T>` now stores `position` and `size` vectors:
  * `rect.left` → `rect.position.x` `rect.width` → `rect.size.x`
  * Build with `sf::Rect<T>{{x, y},{w, h}}`.
  * Intersection: `rect.findIntersection(other)` returns `std::optional<sf::Rect<T>>`.

### 2. Angles
* Every rotation/angle is an `sf::Angle`.
  * Construct with `sf::degrees(f)` or `sf::radians(f)`.
  * Retrieve with `.asDegrees()` / `.asRadians()`.

### 3. Events
* `pollEvent` / `waitEvent` return `std::optional<sf::Event>`.
* Query with
  * `event->is<sf::Event::Closed>()` (no data)
  * `event->getIf<sf::Event::KeyPressed>()` (returns pointer or `nullptr`).
* Alternative: `window.handleEvents(callbacks...)` visitor.

### 4. Enumerations and constants
* All enums are **scoped**. Add the enum name:
  * `sf::Keyboard::A` → `sf::Keyboard::Key::A`.
* Window state: `sf::Style::Fullscreen` → `sf::State::Fullscreen`.
* Primitive: `LinesStrip` → `LineStrip`, `TrianglesStrip` → `TriangleStrip`.
* `PrimitiveType::Quads` no longer exists—draw two triangles.

### 5. Functions & constructors
| SFML 2                                      | SFML 3                    |
|---------------------------------------------|---------------------------|
| `Font::loadFromFile`                        | `Font::openFromFile`      |
| `Texture::create(w,h)`                      | `Texture::resize(w,h)`    |
| `Sound::getLoop / setLoop`                  | `isLooping / setLooping`  |
| `Socket::getHandle`                         | `getNativeHandle`         |
| `WindowBase::getSystemHandle`               | `getNativeHandle`         |
| `RenderWindow::capture()`                   | `Texture tmp; tmp.update(window); Image img = tmp.copyToImage();` |

* `Sound`, `Text`, `Sprite` **must** be constructed with their resource (`SoundBuffer`, `Font`, `Texture`)—no default constructor.
* Resource classes (`SoundBuffer`, `Font`, `Image`, ...) provide throwing file constructors (`Class{"file.ext"}`).

### 6. Types and threading
* `sf::Int32`, `sf::Uint32`, ... → `std::int32_t`, `std::uint32_t`, ...
* `sf::Lock`, `sf::Mutex`, `sf::Thread` → `<thread>` equivalents (`std::lock_guard`, `std::mutex`, `std::thread` or `std::jthread`).
```
- Always format function or method declarations so the first parameter stays on the same line as the function name, and all remaining parameters are placed on their own lines. For example:
```cpp
void run(const event_callback_type &on_event,
         const update_callback_type &on_update,
         const render_callback_type &on_render);
```
- Always use `=` for variable initialization, never brace-initialization. For example:
```cpp
sf::Vector2u resolution = {1280, 720};
unsigned fps = 144;
bool vsync = false;
unsigned anti_aliasing = 8;
```
- Every member variable (whether within a struct, class, or enum) must always have a `@brief` doxygen explanation.
