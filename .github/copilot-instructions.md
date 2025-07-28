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
- The `timeout` command is not available on macOS.

## C++
- Always prefix member variables with `this->`.
- Use `snake_case` for all identifiers, except for class and struct names, which must use `CamelCase`. Avoid `pascalCase` at all costs.
- Use `const`, `constexpr`, and `[[nodiscard]]` whenever possible.
- Class member variables must have a trailing underscore (e.g., `zoom_ratio_`), while struct fields should not (e.g., `zoom_ratio`).
- Use CMake from the `build` directory, e.g., `cmake --build . --parallel`.
- All files in `src/core/` (e.g., `game.hpp`, `backend.hpp`, `ui.hpp`) must be completely independent - they cannot `#include` other core modules. Each core file should only depend on standard library, SFML, and external libraries. This creates a "flat" architecture where higher-level code (like `app.cpp`) can freely import any core modules without circular dependency issues. Think of core modules as independent building blocks that can be mixed and matched by the application layer.
- All files `src/game/` (e.g., `entities.hpp`) can import code from `src/core` but they cannot import any code from `src/game` itself. That way, the modules in the `src/game` directory are independent of each other, but can still use the core modules.
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

When generating automated tests for code, use the C++20 snitch library. Make sure the tag (2nd argument) follows the [DIRECTORY][SUBDIRECTORY][HEADER.HPP] format, e.g., `[src][assets][textures.hpp]`. The full docs can be found below:
```
## Writing Tests

### Test Case Macros

- `TEST_CASE(NAME, TAGS) { ... }` — Standalone test case
- `TEMPLATE_TEST_CASE(NAME, TAGS, TYPES...) { ... }` — Typed test case
- `TEMPLATE_LIST_TEST_CASE(NAME, TAGS, TYPES) { ... }` — Typed test case with reusable type list
- `TEST_CASE_METHOD(FIXTURE, NAME, TAGS) { ... }` — Test case with fixture
- `TEMPLATE_TEST_CASE_METHOD(NAME, TAGS, TYPES...) { ... }` — Typed test case with fixture
- `TEMPLATE_LIST_TEST_CASE_METHOD(NAME, TAGS, TYPES) { ... }` — Typed test case with fixture and type list

#### Example
```cpp
#include <snitch/snitch.hpp>
TEST_CASE("Factorials are computed", "[factorial]") {
    REQUIRE(Factorial(0) == 1);
    REQUIRE(Factorial(1) == 1);
    REQUIRE(Factorial(2) == 2);
}
```

### Test Check Macros

- **Run-time:**
  - `REQUIRE(EXPR);` — Abort test on failure
  - `CHECK(EXPR);` — Continue test on failure
  - `REQUIRE_FALSE(EXPR);`, `CHECK_FALSE(EXPR);`
  - `REQUIRE_THAT(EXPR, MATCHER);`, `CHECK_THAT(EXPR, MATCHER);`
- **Compile-time:**
  - `CONSTEVAL_REQUIRE(EXPR);`, `CONSTEVAL_CHECK(EXPR);`
  - `CONSTEVAL_REQUIRE_FALSE(EXPR);`, `CONSTEVAL_CHECK_FALSE(EXPR);`
  - `CONSTEVAL_REQUIRE_THAT(EXPR, MATCHER);`, `CONSTEVAL_CHECK_THAT(EXPR, MATCHER);`
- **Run-time and Compile-time:**
  - `CONSTEXPR_REQUIRE(EXPR);`, `CONSTEXPR_CHECK(EXPR);`
  - `CONSTEXPR_REQUIRE_FALSE(EXPR);`, `CONSTEXPR_CHECK_FALSE(EXPR);`
  - `CONSTEXPR_REQUIRE_THAT(EXPR, MATCHER);`, `CONSTEXPR_CHECK_THAT(EXPR, MATCHER);`
- **Exception checks:**
  - `REQUIRE_THROWS_AS(EXPR, EXCEPT);`, `CHECK_THROWS_AS(EXPR, EXCEPT);`
  - `REQUIRE_THROWS_MATCHES(EXPR, EXCEPT, MATCHER);`, `CHECK_THROWS_MATCHES(EXPR, EXCEPT, MATCHER);`
  - `REQUIRE_NOTHROW(EXPR);`, `CHECK_NOTHROW(EXPR);`
- **Miscellaneous:**
  - `FAIL(MSG);`, `FAIL_CHECK(MSG);`
  - `SKIP(MSG);`, `SKIP_CHECK(MSG);`


## Advanced Features

### Sections
Use `SECTION("name")` to split a test case into multiple logical sections, sharing setup/teardown logic.

### Captures
Use `INFO(...)` and `CAPTURE(vars...)` to record contextual information for failed checks.

### Tags
Tags are assigned as a string in the test macro, e.g. `[fast][math]`. Special tags:
- `[.]` — hidden test
- `[!mayfail]` — allowed to fail
- `[!shouldfail]` — must fail

### Matchers
Matchers are objects with a `match(obj)` and `describe_match(obj, status)` interface. Built-in matchers:
- `snitch::matchers::contains_substring{"substring"}`
- `snitch::matchers::with_what_contains{"substring"}`
- `snitch::matchers::is_any_of{T...}`

Custom matchers can be defined by implementing the required interface.

### Custom String Serialization
To display custom types in failure messages, define a free function `bool append(snitch::small_string_span, const T&)` in the same namespace as your type.


## Reporters

- Built-in: `console` (default), `teamcity`, `xml`
- Custom reporters can be registered via `REGISTER_REPORTER(NAME, TYPE)` or `REGISTER_REPORTER_CALLBACKS(NAME, INIT, CONFIG, REPORT, FINISH)`
- The main reporter callback signature: `void(const snitch::registry&, const snitch::event::data&) noexcept`


## Command-Line API

- `-h, --help` — Show help
- `-l, --list-tests` — List all tests
- `--list-tags` — List all tags
- `--list-reporters` — List all reporters
- `-r, --reporter <name>` — Select reporter
- `-v, --verbosity <quiet|normal|high|full>` — Set verbosity
- `-o, --output <path>` — Output to file
- `--color <always|default|never>` — Enable/disable colors
- Positional arguments filter tests by name/tag (wildcards supported)


## Filtering Tests

- Use wildcards (`*`) and negation (`~`) to select/exclude tests
- Filters can apply to names or tags
- Multiple filters: space = AND, comma = OR
- Hidden tests (`[.]`) only run if explicitly selected


## Using Your Own main()

By default, snitch defines `main()`. To provide your own, set `SNITCH_DEFINE_MAIN` to `0` and call:
```cpp
int main(int argc, char* argv[]) {
    auto args = snitch::cli::parse_arguments(argc, argv);
    if (!args) return 1;
    snitch::tests.configure(*args);
    return snitch::tests.run_tests(*args) ? 0 : 1;
}
```


## Exception Handling

- With exceptions: test macros abort test case and continue
- Without exceptions: aborts with `std::terminate()`; use `CHECK*` macros for non-aborting checks
```
