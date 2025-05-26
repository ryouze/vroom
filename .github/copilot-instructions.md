## General Programming Guidelines

### All Languages
- Use plain ASCII in logs and comments; no Unicode.
- Use American spelling (e.g., `color`).
- Prefer long, verbose variable names.
- Always give me full code so I can copy paste it; do not omit stuff for brevity or return a git diff.
- Do not split long lines across newlines, keep them long - even if it hurts readability. I especially hate when long comments are split across multiple lines.

### C++
- Always use `this->` when referring to member variables.
- Use snake_case everywhere, apart for class and struct names, which should use CamelCase. I hate pascalCase.
- Use `const` and `constexpr` everywhere whenever possible.
- Use trailing underscore for member variables in classes (e.g., `zoom_ratio_`) but plain names in structs (e.g., `zoom_ratio`).
- Use CMake in the `build` directory.
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
