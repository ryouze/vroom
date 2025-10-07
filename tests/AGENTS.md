# AGENTS.md

**snitch** is a lightweight C++20 testing framework used for unit tests in this project. It provides a Catch2-like API (macros like `TEST_CASE`, `CHECK`, `REQUIRE`, etc.) but with faster compile times and no heap allocations. It's simple, efficient, and well suited for modern C++ projects.

---

## 1. Basic Usage

Include Snitch in your test files:

```cpp
#include <snitch/snitch.hpp>
```

Snitch provides its own `main()` entry point, so **do not define a main function** in your test files.

### Example

```cpp
TEST_CASE("Factorials are computed correctly", "[math][factorial]") {
    REQUIRE(factorial(0) == 1);
    REQUIRE(factorial(5) == 120);
}
```

* The first argument is a descriptive test name.
* The second is an optional tag list, used for filtering (e.g. `[core][math]`).

You can use multiple `TEST_CASE`s per file. Each one runs independently.

---

## 2. Assertions

| Macro                                | Description                              |
| ------------------------------------ | ---------------------------------------- |
| `REQUIRE(expr)`                      | Fails and aborts the test if false.      |
| `CHECK(expr)`                        | Fails but continues running.             |
| `REQUIRE_FALSE(expr)`                | Requires the expression to be false.     |
| `REQUIRE_THROWS_AS(expr, Exception)` | Expects a specific exception type.       |
| `REQUIRE_NOTHROW(expr)`              | Ensures no exception is thrown.          |
| `FAIL("message")`                    | Fails immediately with a custom message. |
| `SKIP("message")`                    | Marks the test as skipped.               |

Use `REQUIRE` for critical checks and `CHECK` when you want to continue testing after a failure.

---

## 3. Sections

Use `SECTION("name")` inside a `TEST_CASE` to separate related scenarios sharing setup code:

```cpp
TEST_CASE("Parsing integers from string", "[util][parse]") {
    std::string input = "123";
    SECTION("Valid input") {
        REQUIRE(parse_int(input) == 123);
    }
    SECTION("Invalid input") {
        input = "abc";
        REQUIRE_THROWS_AS(parse_int(input), std::invalid_argument);
    }
}
```

Each section runs independently with the same setup.

---

## 4. Logging and Context

Use these to add context to test failures:

* `INFO("message")` – Adds contextual info, shown only on failure.
* `CAPTURE(var)` – Logs variable name and value automatically.

Example:

```cpp
for (int i = 0; i < 10; ++i) {
    CAPTURE(i);
    CHECK(compute_value(i) == expected[i]);
}
```

---

## 5. Fixtures

For shared setup/teardown logic, use `TEST_CASE_METHOD`:

```cpp
struct ExampleFixture {
    int base_value = 5;
};

TEST_CASE_METHOD(ExampleFixture, "Fixture test", "[fixture]") {
    REQUIRE(this->base_value == 5);
}
```

Each test gets a new fixture instance for isolation.

---

## 6. Compile-time Tests

Snitch supports constexpr testing:

```cpp
CONSTEXPR_REQUIRE(square(4) == 16);
```

This validates at both compile time and runtime.

---

## 7. Test File Organization

* Mirror the `src/` directory structure under `tests/`.
  Example: `src/core/world.cpp` -> `tests/core/world.test.cpp`
* Use tags that mirror the directory structure for easy filtering in `CMakeLists.txt`, e.g.:
  * Tag `[src][game][entities.hpp]` in `tests/game/entities.test.cpp` file
  * Tag `[src][assets][sounds.hpp]` in `tests/assets/sounds.test.cpp` file
