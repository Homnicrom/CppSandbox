# C++ Sandbox

Inside this project you will find three C++ console projects not connected to each other. Each is a separate exercise that explores specific elements of modern C++ (STL basics, manual RAII/smart-pointer implementation, memory-safety tooling).

Windows with MSVC is the primary toolchain, including for MemorySafetyDrills, given that MSVC has shipped native AddressSanitizer support (`/fsanitize=address`) since VS 2019 16.9, so no separate g++/Clang toolchain is needed there. g++ under Linux/macOS works too and is what CI uses.

The code present in this project has been hand-written before Claude Code was incorporated into it, and in some cases echoes patterns and problems I've experienced professionally during my time at Ubisoft.
Claude Code came in afterward to help assemble this repo, keep it in order, learn new technologies I'm not familiar with (Asan, CMake, CI), and serve as a hands-on way to learn the tool itself: how to fit it into the workflow, and how to get the most out of it.

### Requirements

- **CMake >= 3.20** (as specified by `cmake_minimum_required` in the root `CMakeLists.txt`)
- A **C++17** compiler for LangCatalog
- A **C++23** compiler for RAIIGarage and MemorySafetyDrills. `std::println` specifically needs a `<print>` implementation, which may lag behind general C++23 language support. MSVC in VS 2022 17.5+, GCC 14+ (libstdc++), or Clang 18+ (libc++) could be a reasonable baseline.
- For MemorySafetyDrills' AddressSanitizer build on Windows: **Visual Studio 2019 16.9 or later** (`cl.exe`/`link.exe` native `/fsanitize=address` support), with the "C++ AddressSanitizer" optional component of the "Desktop development with C++" workload installed. No separate g++/Clang toolchain is needed, except on Linux.

The C++ version requirements are enforced automatically through `target_compile_features(... cxx_std_23)`: RAIIGarage declares it `PUBLIC` on its `RAIIGarageLib` target (so both the demo executable and the test executable inherit it), MemorySafetyDrills declares it `PRIVATE` on its own target. Either way `cmake -B build` will fail at **configure time** with a clear error if the selected compiler can't do C++23, rather than silently building with an older standard.

## Build

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
```
For this, make sure Visual Studio 2022 and its Desktop development with C++ workload are installed, or if you are using a different VS version or build system generator, change the generator line in the command above. https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html#manual:cmake-generators(7)
Keep in mind the requirements listed above.

Each project's executable lands under `build/bin/<TargetName>/Debug/`.

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

- A hand-rolled `unique_ptr` (`UniquePtr<T, Deleter>`, with converting move-construction from derived to base). It sparked from working with an in-house `unique_ptr` in Ubisoft's engine; this one is just a simple implementation partly mirroring `unique_ptr`.
- A Meyers-singleton `Registry` that observes vehicle lifetime via `weak_ptr` without owning it. Singleton is a very common pattern in games, and pairing `weak_ptr` with `shared_ptr` showcases resource ownership, which in my opinion is one of the leading efforts when designing a new system.
- A `Dealer` RAII wrapper tying registration to scope, again showing differences between strong and weak references in ownership.
- A rule-of-five `Buffer` class instrumented to show exactly when each special member runs (including copy-elision cases).
- A `constexpr` variadic `Utilities::Max` constrained by concepts, for a modern template constraints showcase. `Numeric` keeps out `bool` and the character types, and `ConsistentSignedness` rejects args packs that mix signed with unsigned.

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
Legend as follows: IEntityListener (IVehicle), PingSystem (module A), GRL.Client (module B).

## MemorySafetyDrills: memory-safety drills (ASan)

Single-file drills built with AddressSanitizer, each demonstrating one classic memory bug before/while you find it.
The current drill (`main.cpp`) is **Drill 1: Dangling reference**: `FindName` returns a `const std::string&` into a `std::vector<Vehicle>`, and a later `push_back` on that vector reallocates its buffer, invalidating the earlier reference before it's read in `main`.

**Windows**: no separate toolchain needed, just the standard build from above (requires VS 2019 16.9+, see Requirements). `MemorySafetyDrills/CMakeLists.txt` turns on `/fsanitize=address` for MSVC by default:

```powershell
cmake --build build --config Debug --target MemorySafetyDrills
build\bin\MemorySafetyDrills\Debug\MemorySafetyDrills.exe
```

**Linux/macOS**: use the line from the file's own header comment:

```bash
g++ -std=c++23 -fsanitize=address -g main.cpp -o d1
./d1
```

Either way, ASan should report a use-after-free on the reallocated vector storage.

## Tests

RAIIGarage has a GoogleTest suite under `source/RAIIGarage/tests/`: 40 cases across five suites (`UniquePtrTest`, `VehiclesHelperTest`, `RegistryTest`, `DealerTest`, `BufferTest`), plus `UtilitiesMaxTests.cpp`, which is `static_assert`-only on purpose since `Utilities::Max` is `constexpr` and a runtime `EXPECT` would be the weaker check.

GoogleTest (v1.18.0) is pulled in by the root `CMakeLists.txt` via `FetchContent`, so there is nothing to install by hand: configuring the project downloads it.

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

The test executable is a target of its own (`RAIIGarageTests`, built from the shared `RAIIGarageLib` object library so it compiles the project's sources under exactly the same settings the demo does), so you can also run it directly:

```powershell
build\bin\RAIIGarageTests\Debug\RAIIGarageTests.exe
```

Building `RAIIGarage` depends on the `RunRAIIGarageTests` custom target, so the demo executable won't build unless the suite has run and passed first.

CI additionally runs the whole suite in a single shared process in random order (`--gtest_shuffle --gtest_repeat=5`). CTest runs every case in its own process, which makes cross-test contamination through the `Registry` singleton invisible to it; `Registry::Reset()` in `RegistryTest`'s `SetUp` is what prevents that contamination, and the shuffled single-process run is what actually proves it.

## CI

`.github/workflows/ci.yml`, on push and PR to `main`:

- **build-windows**: configure, build every target in Debug|x64, `ctest`, then the shuffled single-process run described above.
- **asan-windows**: builds MemorySafetyDrills with MSVC's native `/fsanitize=address` and **fails if the drill exits 0**. The drill is supposed to trip ASan, so a clean exit means the bug was accidentally fixed.
- **asan-linux**: compiles `source/MemorySafetyDrills/main.cpp` directly with `g++-14 -std=c++23 -fsanitize=address -g` (no CMake), with the same nonzero-exit assertion.

## License

MIT. See [LICENSE](LICENSE).
