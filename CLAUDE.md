# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A CMake project (`CMakeLists.txt` at the repo root, which `add_subdirectory(source projects)`s into `source/CMakeLists.txt`, which in turn `add_subdirectory()`s each project's own `CMakeLists.txt`) containing three **independent, unrelated** C++ console projects living under `source/{LangCatalog,RAIIGarage,MemorySafetyDrills}/`. There is no shared purpose across them: each is a separate sandbox/exercise. Don't assume changes in one project are relevant to another.

- **LangCatalog**: reads `languages.txt`, parses `<name> <designer words...> <year>` lines into a `language` struct, prints them as CSV. Plain C++17-style code, no dependencies beyond the STL.
- **RAIIGarage**: a C++23 language-features sandbox (smart pointers, RAII, singletons, concepts). See "RAIIGarage architecture" below.
- **MemorySafetyDrills**: single-file memory-safety "drills" meant to be built with AddressSanitizer. The current file's content is "Drill 1: Dangling reference" (a reference into a `std::vector` invalidated by reallocation) in `main.cpp`. Expect this file's content to be swapped out for different drills over time; check the comment header at the top of the file, not the filename, to know which drill is currently in place.

## Build

CMake. From the repo root:

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```

Each project's executable lands under `build/bin/<ProjectName>/Debug/` (set explicitly per-target via `set_target_properties(... RUNTIME_OUTPUT_DIRECTORY ...)` in the root `CMakeLists.txt`). You can also open the `build/` folder in Visual Studio (or generate the solution above and open `build\CppSandbox.sln`, named after the root `project(CppSandbox ...)` call rather than any individual sub-project) to build/run/debug individual targets from the IDE. The root-level `CreateVisualStudioSolution.bat` runs the `cmake -B build ...` configure step for you if you just want the solution generated without typing the command by hand.

Each project's own `CMakeLists.txt` (under `source/<Project>/`) sets `cxx_std_17` for LangCatalog and `cxx_std_23` for RAIIGarage/MemorySafetyDrills (needed for `std::println`, concepts, etc.) via `target_compile_features`, so there's no configuration-specific include-path or language-standard quirk to worry about the way a hand-edited `.vcxproj` would have: it applies uniformly across Debug/Release and Win32/x64. `RAIIGarage`'s `#include <RAIIGarage/Foo.h>`-style includes resolve via `target_include_directories(... PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)` in its `CMakeLists.txt` (the parent of `source/RAIIGarage`, so the `RAIIGarage/` prefix in includes matches the on-disk folder name).

MemorySafetyDrills's drill files carry their own build recipe in a header comment; prefer it over guessing new flags.

- **Windows**: `cl.exe` has had native `/fsanitize=address` support since VS 2019 16.9 — no separate compiler needed, the normal `cmake -B build -G "Visual Studio 17 2022" -A x64` / `cmake --build build --config Debug` flow from the Build section above already produces an ASan-instrumented binary, including in Debug config. `MemorySafetyDrills/CMakeLists.txt` adds `/fsanitize=address` for MSVC and, since VS's own auto-copy-the-runtime-DLL behavior only fires through its `EnableASan` project property (which plain `target_compile_options` doesn't set), a `POST_BUILD` custom command copies the matching `clang_rt.asan*dynamic-x86_64.dll` next to the `.exe` itself. (Do *not* reach for Clang or MinGW/MSYS2 g++ here: MinGW-w64 g++ doesn't ship `libasan` at all — `-fsanitize=address` compiles but fails to link with `cannot find -lasan` — and getting real ASan working via VS's bundled Clang requires a non-default Ninja+plain-clang++ setup to work around MSBuild always linking via `lld-link.exe` directly, which silently drops `-fsanitize=address`. Both are strictly more setup than the MSVC path above for the same result.)
- **Linux**: plain `g++ -fsanitize=address` works out of the box (Ubuntu's g++ ships `libasan`), matching `.github/workflows/ci.yml`'s Linux job. `MemorySafetyDrills/CMakeLists.txt`'s `MEMSAFETYDRILLS_ENABLE_ASAN` option (on by default whenever `CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"`) means a plain `cmake --build` there also produces an ASan-instrumented binary with no extra flags needed.

The root `CMakeLists.txt` also `FetchContent`s GoogleTest, but as of this writing no project actually has any `TEST()`/`TEST_F()` cases: it's wired up but unused. There is no lint config. `.github/workflows/ci.yml` defines a Windows/MSVC build job, a Windows/MSVC-native AddressSanitizer job, and a Linux/g++ AddressSanitizer job, the latter two both exercising the MemorySafetyDrills drill. The repo has a GitHub remote (`origin` → `github.com/Homnicrom/CppSandbox`) with a tracked `main` branch, so these jobs run on push/PR to `main`.

## Running

- LangCatalog expects `languages.txt` to be present in the working directory (it's copied next to the project; when running via Visual Studio the working directory is the project folder).
- RAIIGarage's `main()` currently runs all demo sections back to back (Utilities::Max, IVehicle/Registry/Dealer, the custom `UniquePtr`, and Buffer rule-of-five). Expect this to get pared down to one active block at a time as it's edited; when working on a specific feature, feel free to comment out unrelated sections rather than adding new demo code elsewhere.

## RAIIGarage architecture

This project explores manual memory management and RAII patterns; understanding it requires reading across several headers together:

- **`UniquePtr.h`**: a hand-rolled `unique_ptr` with a pluggable deleter (`DefaultDelete<T>` by default), supporting converting move-construction from derived to base (`UniquePtr<Derived,...>` → `UniquePtr<Base,...>`).
- **`Vehicles.h`/`.cpp`**: `IVehicle` is an abstract base with a **private** pure-virtual `DeleteVehicle()`; concrete vehicles (`Car`, `Van`, `Forklift`, `Truck`) implement it as `delete this`. Construction goes only through `IVehicle::Helper` (a nested factory/deleter class declared `friend` by each vehicle so it alone can call their private `Setup(...)`), which offers three creation flavors: `CreateVehicle` (`std::unique_ptr` with `Helper` as deleter), `CreateVehicleCustom` (the project's own `UniquePtr` with `Helper` as deleter), and `CreateVehicleShared` (`std::shared_ptr`). `Helper::operator()` is what actually invokes the private `DeleteVehicle()`; this is why vehicles can't be deleted except through the smart pointers this factory hands out.
- **`Registry.h`/`.cpp`**: a Meyers singleton (`GetInstance()`) holding `std::weak_ptr<IVehicle>` for every live vehicle, so it observes vehicle lifetime without owning it. `CallVehicles<T>()` is templated to optionally filter by dynamic type via `typeid`.
- **`Dealer.h`/`.cpp`**: a small RAII wrapper: holds a `shared_ptr<IVehicle>`, registers it with `Registry` on construction and removes it on destruction. Demonstrates scope-based lifetime tied to the registry.
- **`Buffer.h`**: a minimal rule-of-five class (copy/move ctor+assignment, all instrumented with `std::println`) used to observe when each special member actually runs (including copy elision cases).
- **`Utilities.h`**: `Utilities::Max(...)`, a `constexpr` variadic max over an arithmetic-type-constrained parameter pack (via a `ComparableNumeric` concept).
- **`ScopedTimer.h`**: RAII stopwatch that prints elapsed milliseconds on destruction; used at the top of `main()`.

When modifying vehicle types or the factory, keep the private-`Setup`/private-`DeleteVehicle` + `friend IVehicle` pattern intact; it's what forces all vehicle creation/destruction through `IVehicle::Helper`.

# AI Collaboration Context

*Written following the AI Fluency framework (Delegation, Description, Discernment, Diligence).*

---

## 1. Who I am and what I do

I'm a C++/C# developer focused on video games. I'm working on a **C++ learning project** with a dual goal: (a) building something functional and (b) learning to use Claude and related technologies along the way.

- **Scope:** practice project, personal use. Not production code, not third-party code.
- **My role:** lead developer and student. I handle everything: planning, development, AI interaction, and iteration.
- **What AI solves for me:** my initial unfamiliarity with Claude, while reinforcing knowledge I already have.

## 2. How I want you to collaborate with me

My default mode of work is **guidance, not substitution**. In order of preference:

1. **Guidance, advice, and resources** over closed-form solutions.
2. **Explain the why**, not just the what. I prefer to understand the reasoning and the alternatives you discarded.
3. **Complete solutions only when:**
   - I explicitly ask for them, or
   - they resolve a real underlying problem (not a surface-level symptom).

I don't delegate to "make the problem go away." I delegate to **free up mental bandwidth**: if you handle the repetitive and basic work and guide me through implementation, I spend that time on what matters, the **relationships between systems** and the higher-level design decisions, which I want to keep making myself.

## 3. Division of responsibilities

| Prefer to delegate to you | I keep for myself |
| --- | --- |
| Repetitive, mechanical tasks (boilerplate, scaffolding) | Architecture and design decisions |
| Explaining unfamiliar concepts and APIs | How systems relate to one another |
| Proposing options with their trade-offs | Choosing between options |
| Locating resources and documentation | Validating and understanding what I learn |
| First drafts of code for me to review | Accepting, rejecting, or modifying each change |

## 4. How to give me useful answers

- **Be concise and get to the point.** Avoid filler.
- **Offer options with their trade-offs** when there's more than one reasonable approach, rather than imposing a single one.
- **Flag your assumptions**, especially when the prompt is ambiguous. I prefer an explicit assumption over an unnecessary question, but ask when the ambiguity is fundamental.
- **Don't make things up.** If you don't know or aren't sure, say so. I'd rather have "I don't know, verify it here" than a plausible but false answer.
- **Treat me as someone who wants to learn**, not someone whose problem needs solving for them. When you show a solution, explain enough that I can reproduce the reasoning without you next time.

## 5. Discernment and verification (my part)

I assume **ultimate responsibility for the code is mine**. Therefore:

- I manually review every change before accepting it.
- I expect to be able to question your proposals; you should also flag when something is debatable or carries risk.
- When something is wrong, I want to know **why** it was wrong, not just the fix.

---

*This document describes default preferences. For a specific task I may explicitly request a different mode (e.g. "this time, give me the direct and complete solution").*

