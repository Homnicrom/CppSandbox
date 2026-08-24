# Static Analysis

Sanitizers only catch bugs on paths that run. Static analyzers reason about paths that never executed, catching latent bugs a test suite misses. Use both: they overlap little.

## clang-tidy

The most useful safety-relevant check groups:

- **`bugprone-*`** — likely-bug patterns: `bugprone-use-after-move`, `bugprone-dangling-handle` (dangling `string_view`/references), `bugprone-integer-division`, `bugprone-branch-clone`, `bugprone-unhandled-self-assignment`.
- **`cppcoreguidelines-*`** — Core Guidelines enforcement: `cppcoreguidelines-owning-memory`, `cppcoreguidelines-pro-bounds-*` (array indexing safety), `cppcoreguidelines-pro-type-reinterpret-cast`, `cppcoreguidelines-special-member-functions` (Rule of Five).
- **`clang-analyzer-*`** — the deeper path-sensitive analyzer: `clang-analyzer-core.NullDereference`, `clang-analyzer-cplusplus.NewDelete` (use-after-free / double-free), `clang-analyzer-cplusplus.Move`, `clang-analyzer-unix.Malloc`.

Run:
```bash
clang-tidy -p build/ --checks='bugprone-*,cppcoreguidelines-*,clang-analyzer-*' source/**/*.cpp
```
It needs `compile_commands.json` (generate with `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`). A `.clang-tidy` file at the repo root pins the check list and warning-as-error policy per project.

## MSVC /analyze

MSVC's built-in static analyzer catches many of the same lifetime/bounds/null issues on Windows without Clang:
```powershell
cl /analyze /W4 main.cpp
```
In CMake: `target_compile_options(mytarget PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/analyze>)`. Slower to compile; run it as its own CI job rather than every build.

## Compiler warnings as the first analyzer

Warnings are free static analysis. Treat the safety-relevant ones as errors:

- **GCC/Clang:** `-Wall -Wextra -Wshadow -Wconversion -Wold-style-cast -Wnull-dereference -Wuninitialized -Wdangling-reference` (GCC 13+), and `-Werror` in CI (not in day-to-day local builds, where it slows iteration).
- **MSVC:** `/W4` (or `/Wall`, noisier), `/w14640` and related, `/permissive-` for standard conformance, `/WX` for warnings-as-errors in CI.

`-Wuninitialized`, `-Wdangling-reference`, and `-Wreturn-stack-address` directly target memory-safety mistakes and cost nothing.

## How to combine

1. Warnings-as-errors in CI catch the cheap cases at compile time.
2. clang-tidy / `/analyze` as a separate CI job catch path-sensitive latent bugs.
3. ASan/UBSan/TSan jobs confirm the bugs that actually manifest at runtime.

No single tier is sufficient: warnings miss cross-function lifetime bugs, static analysis has false positives and misses runtime-only races, sanitizers miss unexecuted paths. Recommend the layered setup, and when reviewing a bug, note which tier would have caught it earliest so the user closes that gap.
