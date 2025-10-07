When generating tests for code, use the C++20 snitch library. Make sure the tag (2nd argument) follows the `[DIRECTORY][SUBDIRECTORY][HEADER.HPP]` format, e.g., `[src][assets][textures.hpp]`. The full docs can be found below:

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
