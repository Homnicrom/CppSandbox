# Modern C++ Idiom Cheatsheet

Match the project's standard. Do not introduce features from a newer standard than the build targets. Detect the standard from build flags or usage before recommending.

## C++17

- **Structured bindings:** `auto [key, val] = pair;`
- **`if`/`switch` with initializer:** `if (auto it = m.find(k); it != m.end())`
- **`std::optional<T>`** for "maybe a value" instead of sentinels or out-params.
- **`std::variant<Ts...>`** + `std::visit` for closed type sets instead of tagged unions.
- **`std::string_view`** for non-owning string params (never store one past the source's lifetime).
- **`[[nodiscard]]`, `[[maybe_unused]]`, `[[fallthrough]]`.**
- **Fold expressions** for variadic templates: `(args + ...)`.

## C++20

- **Concepts** replace SFINAE and `enable_if`. Constrain templates readably:
  ```cpp
  template<typename T, typename U>
  concept ComparableNumeric =
      std::is_arithmetic_v<T> && std::is_arithmetic_v<U> &&
      std::totally_ordered_with<T, U>;

  template<typename T, typename U> requires ComparableNumeric<T, U>
  std::common_type_t<T, U> Max(T t, U u) { return (t > u) ? t : u; }
  ```
  Prefer a named concept over an inline `requires` clause when the constraint is reused or non-trivial.
- **Ranges & views:** `std::ranges::sort(v)`, `v | std::views::filter(...) | std::views::transform(...)`. Lazy, composable, no manual iterator pairs.
- **`std::span<T>`** for non-owning contiguous views over arrays/vectors.
- **`std::print` / `std::println`** (via `<print>`, widely available with C++20/23 toolchains) over `iostream`: type-safe, faster, less verbose.
  ```cpp
  std::println("{0} {1}", name, speed);
  ```
- **Designated initializers:** `Config c{ .width = 80, .height = 24 };`
- **`consteval` / `constinit`**, three-way comparison `operator<=>` (the spaceship) to synthesize all six comparisons.

## C++23

- **`std::expected<T, E>`** for recoverable errors without exceptions.
- **`std::print`** standardized in `<print>`.
- **`std::mdspan`**, ranges additions (`std::views::zip`, `enumerate`), `std::flat_map`.

## Anti-patterns to flag

- Raw index `for` loops where a range-for or algorithm reads clearer.
- `iostream` boilerplate where `std::print` fits the target standard.
- `#define` constants instead of `constexpr` / `enum class`.
- C-style casts instead of `static_cast` / `reinterpret_cast` / `const_cast`.
- Plain `enum` leaking names into the enclosing scope; prefer `enum class`.
- Manual SFINAE where a concept would be clearer (C++20+).
- Returning by `const` value (blocks moves, no benefit).
- `std::endl` in a loop (forces a flush); use `'\n'`.
- Passing `std::string`/large types by value when a `const&` suffices, or by `const&` when the callee needs its own copy anyway (then take by value and move).
