---
name: cmake-build-and-test
description: Handles all CMake interactions in this C++ sandbox repo (CppSandbox, containing LangCatalog, RAIIGarage, MemorySafetyDrills). Use whenever the task involves configuring, building, rebuilding, cleaning, running, or testing any project via CMake, adding a target or GoogleTest case, enabling or diagnosing AddressSanitizer, running the CI build locally, or debugging a CMake configure/build/link failure. Trigger this even when the user just says "build it", "run the tests", "run the ASan drill", "why won't this compile", or names a project (LangCatalog/RAIIGarage/MemorySafetyDrills) without saying "CMake" explicitly.
allowed-tools: Bash, Read, Edit, Glob, Grep
---

# cmake-build-and-test

Canonical workflow for every CMake interaction in this repository. Follow it instead of improvising flags, generators, or ad hoc `cl.exe`/`g++` invocations, because the repo has non-obvious constraints (per-target output dirs, MSVC-only ASan runtime copy, C++ standard set per project) that a fresh guess will get wrong.

## Repository layout (what CMake sees)

Root `CMakeLists.txt` declares `project(CppSandbox ...)` and `add_subdirectory(source)`, which adds each project's own `CMakeLists.txt`:

- **LangCatalog** (`cxx_std_17`): reads `languages.txt` from its working directory. STL only.
- **RAIIGarage** (`cxx_std_23`): language-features sandbox. Includes resolve via `target_include_directories(... PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)`.
- **MemorySafetyDrills** (`cxx_std_23`): single-file ASan drill. Content is swapped between drills; the active drill is identified by the comment header in `main.cpp`, not the filename.

The root also `FetchContent`s GoogleTest, but no `TEST()`/`TEST_F()` cases exist yet. It is wired up but unused. There is no lint config.

Per-target executables land under `build/bin/<ProjectName>/<Config>/` (set explicitly via `set_target_properties(... RUNTIME_OUTPUT_DIRECTORY ...)` in the root file). Do not assume the default `build/<Config>/` location.

## Golden path: configure and build

Always configure into a `build/` directory (out-of-source). Do not run `cmake` directly in the repo root.

**Windows (default, MSVC / Visual Studio 17 2022):**
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```
This is a multi-config generator, so `--config` is chosen at build time, not configure time. Use `--config Release` for release builds without reconfiguring.

**Linux (g++):**
```bash
cmake -B build
cmake --build build
```
Single-config generator here; if a specific config is needed, pass `-DCMAKE_BUILD_TYPE=Debug` at configure time.

The generated Visual Studio solution is `build\CppSandbox.sln` (named after the root `project()` call, not any sub-project). Opening the `build/` folder in Visual Studio also works.

## Building or running a single project

Build one target instead of the whole tree:
```powershell
cmake --build build --config Debug --target RAIIGarage
```

Run it from its per-target output dir. LangCatalog needs `languages.txt` in its working directory (it is copied next to the project; from Visual Studio the working directory is the project folder):
```powershell
.\build\bin\LangCatalog\Debug\LangCatalog.exe
```

RAIIGarage's `main()` runs all demo sections back to back. When working on one feature, comment out unrelated sections rather than adding new demo code elsewhere.

## Clean rebuild

Prefer deleting the build tree over guessing at stale-cache flags:
```powershell
Remove-Item -Recurse -Force build   # PowerShell
```
```bash
rm -rf build                        # bash
```
Then reconfigure. Reach for a full clean when the CMake cache holds a stale generator, compiler, or standard, or when include paths behave inconsistently.

## AddressSanitizer (MemorySafetyDrills)

ASan is the point of MemorySafetyDrills. The default configure/build flow already produces an instrumented binary. Do NOT switch compilers to enable it.

- **Windows / MSVC**: native `/fsanitize=address` (VS 2019 16.9+). The normal `Visual Studio 17 2022` flow instruments the binary, including Debug. `MemorySafetyDrills/CMakeLists.txt` adds `/fsanitize=address` and a `POST_BUILD` command that copies the matching `clang_rt.asan*dynamic-x86_64.dll` next to the `.exe` (VS's own runtime auto-copy only fires through its `EnableASan` project property, which plain `target_compile_options` does not set). If the exe fails to start with a missing-DLL error, that copy step is what to check.
- **Linux / g++**: `MEMSAFETYDRILLS_ENABLE_ASAN` is on by default when `CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"`, so a plain build is instrumented with no extra flags. Ubuntu's g++ ships `libasan`.
- **Do not** reach for Clang or MinGW/MSYS2 g++ here. MinGW-w64 g++ has no `libasan` (links fail with `cannot find -lasan`), and VS's bundled Clang needs a non-default Ninja + plain clang++ setup to avoid `lld-link.exe` silently dropping `-fsanitize=address`. Both are more setup than the MSVC path for the same result.

Each drill file carries its own build recipe in a header comment. Prefer that recipe over inventing new flags.

Run a drill and expect ASan to report the fault:
```powershell
.\build\bin\MemorySafetyDrills\Debug\MemorySafetyDrills.exe
```

## Tests (GoogleTest)

GoogleTest is fetched via `FetchContent` but currently has no test cases. When adding the first test:

1. Confirm the target links GoogleTest (`gtest_main`) in the relevant `CMakeLists.txt`, and that `enable_testing()` and `include(GoogleTest)` + `gtest_discover_tests(<target>)` are present (add them if missing rather than assuming).
2. Reconfigure so CMake picks up the new test target.
3. Run via CTest from the build dir:
```powershell
ctest --test-dir build --config Debug --output-on-failure
```

Flag to the user when wiring is missing rather than silently scaffolding a whole test harness. Offer the change and its trade-offs first.

## Running CI locally

`.github/workflows/ci.yml` defines three jobs: Windows/MSVC build, Windows/MSVC-native ASan, and Linux/g++ ASan (the last two exercise the MemorySafetyDrills drill). It has not run yet because the repo is not on GitHub. To reproduce a job locally, run that job's configure/build/run sequence with the matching generator and config from the sections above.

## Diagnosing failures

Work from the actual error, not a guess. Common cases here:

- **`std::println` / concepts fail to compile in RAIIGarage or MemorySafetyDrills**: the C++23 standard is not applied. Check `target_compile_features(... cxx_std_23)` in that project's `CMakeLists.txt` and that the toolchain supports it. Do not add `/std:c++latest` manually as a workaround before checking.
- **`#include <RAIIGarage/Foo.h>` not found**: the `PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..` include dir is missing or the on-disk folder name no longer matches the `RAIIGarage/` include prefix.
- **ASan exe fails to launch on Windows (missing `clang_rt.asan*.dll`)**: the `POST_BUILD` copy command did not run or targeted the wrong runtime. Rebuild the target; confirm the DLL sits next to the `.exe`.
- **`cannot find -lasan` on Windows**: a MinGW/MSYS2 g++ is being used. Switch back to the MSVC flow.
- **Stale generator/compiler/standard**: delete `build/` and reconfigure (see Clean rebuild).

When a proposed fix is debatable or carries risk, say so and present the options rather than applying the most invasive one.
