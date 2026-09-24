# C++ Sandbox

Inside this project you will find three C++ console projects not connected to each other. Each is a separate exercise that explores specific elements of modern C++ (STL basics, manual RAII/smart-pointer implementation, memory-safety tooling).

Two toolchains are supported: Windows with MSVC and Linux with `g++-14`. Each has a CMake preset and a CI job that builds every target and runs the tests. Neither needs an extra toolchain for MemorySafetyDrills' AddressSanitizer build.

The code present in this project has been hand-written before Claude Code was incorporated into it, and in some cases echoes patterns and problems I've experienced professionally during my time at Ubisoft.
Claude Code came in afterward to help assemble this repo, keep it in order, learn new technologies I'm not familiar with (Asan, CMake, CI), and serve as a hands-on way to learn the tool itself: how to fit it into the workflow, and how to get the most out of it.

## Requirements

- **CMake >= 3.21** (as specified by `cmake_minimum_required` in the root `CMakeLists.txt`, and required by `CMakePresets.json`)
- A **C++17** compiler for LangCatalog
- A **C++23** compiler for RAIIGarage and MemorySafetyDrills. `std::println` specifically needs a `<print>` implementation, which may lag behind general C++23 language support. MSVC in VS 2022 17.5+, GCC 14+ (libstdc++), or Clang 18+ (libc++) could be a reasonable baseline.
- For MemorySafetyDrills' AddressSanitizer build on Windows: **Visual Studio 2022** (as pinned by the `windows` preset; ASan itself needs only 16.9+), with the "C++ AddressSanitizer" component of the "Desktop development with C++" workload. No MinGW or Clang install is needed — `cl.exe` handles `/fsanitize=address` natively.
- On Linux: **`g++-14`** (for C++23) and **Ninja**, as expected by the `linux` preset below.

The C++ version requirements are enforced automatically through `target_compile_features(... cxx_std_23)`: RAIIGarage declares it `PUBLIC` on its `RAIIGarageLib` target (so both the demo executable and the test executable inherit it), MemorySafetyDrills declares it `PRIVATE` on its own target. Either way it fails at **configure time** with a clear error if the selected compiler can't do C++23, rather than silently building with an older standard.

GCC 13 slips past that check: it accepts `-std=c++23` but ships no `<print>`, so the build would only fail later on a missing include. The root `CMakeLists.txt` rejects GCC older than 14 explicitly for that reason.

## Build

`CMakePresets.json` carries one preset per platform, so the same three commands work on both:

```bash
cmake --preset <windows|linux>          # configure
cmake --build --preset <...>-debug      # build every target
ctest --preset <...>-debug              # run the tests
```

- **`windows`** → Visual Studio 17 2022, x64. VS reads the presets directly (*File → Open → Folder*); `CreateVisualStudioSolution.bat` is a double-click wrapper around `cmake --preset windows` that leaves you `build\CppSandbox.sln`.
- **`linux`** → Ninja Multi-Config with `g++-14`. Multi-Config is deliberate: it keeps the configs and the output layout identical to the Visual Studio generator, so there is one set of instructions rather than one per platform.

Each project's executable lands under `build/bin/<TargetName>/<Config>/`, which is `Debug/` for the presets above.

### VS Code

`.vscode/` is set up for the CMake Tools extension (recommended on first open), which drives the presets above, so build, debug and the Test Explorer match the command line.

## LangCatalog: text parsing / STL basics

Reads `languages.txt`, parses `<name> <designer words...> <year>` lines into a small struct, prints them as CSV. Plain C++17-style code, STL only: streams, `std::vector`, manual tokenizing.

Sample output:

```
C,Kernighan & Ritchie,1970
C++,Stroustrup,1979
Java,Gosling,1991
C#,Hejlsberg,1999
Python,van Rossum,1991
```

## RAIIGarage: RAII, smart pointers, singleton, concepts

The main sandbox.

- A hand-rolled `unique_ptr` (`UniquePtr<T, Deleter>`, with converting move-construction from derived to base). It sparked from working with an in-house `unique_ptr` in Ubisoft's engine; though this one is just a simple implementation partly mirroring `std::unique_ptr`.
- A Meyers-singleton `Registry` that observes vehicle lifetime via `weak_ptr` without owning it. Singleton is a very common pattern in games, and pairing `weak_ptr` with `shared_ptr` showcases resource ownership, which in my opinion is one of the leading efforts when designing a new system.
- A `Dealer` RAII wrapper tying registration to scope, again showing differences between strong and weak references in ownership.
- A rule-of-five `Buffer` class instrumented to show exactly when each special member runs (including copy-elision cases).
- A `constexpr` variadic `Utilities::Max` constrained by concepts, for a modern template constraints showcase. `Numeric` keeps out `bool` and the character types, and `ConsistentSignedness` rejects args packs that mix signed with unsigned.
- A `ScopedTimer` that prints its lifetime on destruction. It sits at the top of `main()` and is also a member of every `IVehicle`, so vehicle destruction is visible in the output too.

### The IVehicle factory

A vehicle hierarchy (`Car`, `Van`, `Forklift`, `Truck`) whose construction/destruction is forced through a private-`Setup`/private-`DeleteVehicle` + `friend` factory pattern.
This case comes from a pattern I had to come up with at Ubisoft to avoid cyclical dependencies between code modules.

Think of a **module A** that holds some **local gameplay** code and needs to access a **module B** which handles **replication/network functionalities**, but B also needs A to connect replicated data with local gameplay related to A. That's a textbook cyclical dependency.

So here comes the **IVehicle interface** (representing the real case solution which can be checked at the end link) that needs to be used by both module A and B.
It is created inside its own **module C**, which does not depend in either module A or B and breaks the cycle via dependency inversion.

This way, IVehicle's derived classes can be created in any module to implement that module's specific functionalities; then that derived class can be used by other modules that would otherwise be dependent on the first by directly using the IVehicle interface.
IVehicle::Helper hides derived class construction and destruction, acting as a factory, and leaves IVehicle itself with pure virtual methods as its API.

Going back to modules A and B, now a class in module B can construct a derived class from IVehicle that uses module B's network functionality and pass it for module A to use through the interface without dependency issues.

This case is directly linked with my professional technical design documents that can be found here: https://www.flipsnack.com/homni/diego-v-zquez-garrido-portfolio-t95olqqm8y.
Legend as follows: IEntityListener (IVehicle), PingSystem (module A), ReplicationClient (module B).

## MemorySafetyDrills: memory-safety drills (ASan)

Single-file drills built with AddressSanitizer, each demonstrating one classic memory bug before/while you find it.
The current drill (`main.cpp`) is **Dangling reference**: `FindName` returns a `const std::string&` into a `std::vector<Vehicle>`, and a later `push_back` on that vector reallocates its buffer, invalidating the earlier reference before it's read in `main`.

**Either platform**: no separate toolchain, just the standard build from above (Windows ASAN requires VS 2019 16.9+, see Requirements). `MemorySafetyDrills/CMakeLists.txt` turns on `/fsanitize=address` for MSVC and `-fsanitize=address` for GCC/Clang by default:

```bash
cmake --build --preset <windows|linux>-debug --target MemorySafetyDrills
build/bin/MemorySafetyDrills/Debug/MemorySafetyDrills
```

**Without CMake** (Linux): the line from the file's own header comment, run from the drill's folder. Use `g++-14` rather than plain `g++`: the distro default is still 13 on the baseline above, and it has no `<print>`.

```bash
cd source/MemorySafetyDrills
g++-14 -std=c++23 -fsanitize=address -g main.cpp -o d1
./d1
```

Either way, ASan should report a use-after-free on the reallocated vector storage.

## Tests

RAIIGarage has a GoogleTest suite under `source/RAIIGarage/tests/`: 40 cases across five suites (`UniquePtrTest`, `VehiclesHelperTest`, `RegistryTest`, `DealerTest`, `BufferTest`), plus `UtilitiesMaxTests.cpp`, which is `static_assert`-only on purpose since `Utilities::Max` is `constexpr` and a runtime `EXPECT` would be the weaker check.

GoogleTest (v1.18.0) is pulled in by the root `CMakeLists.txt` via `FetchContent`, so there is nothing to install by hand: configuring the project downloads it.

```bash
ctest --preset <windows|linux>-debug
```

The test executable is a target of its own (`RAIIGarageTests`, built from the shared `RAIIGarageLib` object library so it compiles the project's sources under exactly the same settings the demo does), so you can also run it directly:

```
build\bin\RAIIGarageTests\Debug\RAIIGarageTests.exe   :: Windows
build/bin/RAIIGarageTests/Debug/RAIIGarageTests       # Linux
```

Building `RAIIGarage` depends on the `RunRAIIGarageTests` custom target, so the demo executable won't build unless the suite has run and passed first.

CI additionally runs the whole suite in a single shared process in random order (`--gtest_shuffle --gtest_repeat=5`). CTest runs every case in its own process, which makes cross-test contamination through the `Registry` singleton invisible to it; `Registry::Reset()` in `RegistryTest`'s `SetUp` is what prevents that contamination, and the shuffled single-process run is what actually proves it.

## CI

`.github/workflows/ci.yml`, on push and PR to `main`:

- **build-windows** / **build-linux**: configure, build every target in Debug, `ctest`, then the shuffled single-process run described above. Both drive the presets, so CI and a local checkout cannot drift apart.
- **asan-windows**: builds MemorySafetyDrills with MSVC's native `/fsanitize=address` and **fails if the drill exits 0**. The drill is supposed to trip ASan, so a clean exit means the bug was accidentally fixed.
- **asan-linux**: compiles `source/MemorySafetyDrills/main.cpp` directly with `g++-14 -std=c++23 -fsanitize=address -g` (no CMake), with the same nonzero-exit assertion.

## License

MIT. See [LICENSE](LICENSE).
