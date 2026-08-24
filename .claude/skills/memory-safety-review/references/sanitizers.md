# Sanitizers

Sanitizers are compile-time instrumentation that catches bugs at runtime on executed paths. Build with the sanitizer, run the triggering path, read the report. They only see code that actually runs, so pair them with static analysis.

## Which sanitizer for which bug

| Sanitizer | Catches | Flag |
| --- | --- | --- |
| AddressSanitizer (ASan) | heap/stack/global overflow, use-after-free, use-after-return, double-free | `-fsanitize=address` / `/fsanitize=address` |
| LeakSanitizer (LSan) | memory leaks | bundled with ASan on Linux; `ASAN_OPTIONS=detect_leaks=1` |
| UndefinedBehaviorSanitizer (UBSan) | signed overflow, invalid casts, misaligned access, null deref, more | `-fsanitize=undefined` |
| ThreadSanitizer (TSan) | data races, some deadlocks | `-fsanitize=thread` |
| MemorySanitizer (MSan) | uninitialized reads (Clang only, Linux) | `-fsanitize=memory` |

**ASan and TSan are mutually exclusive in one build.** UBSan combines with ASan (`-fsanitize=address,undefined`). Always build sanitized targets with `-g` for symbolized traces, and `-O1 -fno-omit-frame-pointer` for readable stacks without losing all speed.

## Cross-compiler invocation

**GCC / Clang (Linux, out of the box on Ubuntu, which ships `libasan`):**
```bash
g++ -std=c++23 -g -O1 -fno-omit-frame-pointer -fsanitize=address main.cpp -o app
./app                       # ASan + LSan report on exit
```
UBSan: `-fsanitize=undefined -fno-sanitize-recover=all` (abort instead of just logging). TSan: `-fsanitize=thread`.

**MSVC (Windows):** `cl.exe` has native `/fsanitize=address` since VS 2019 16.9, including in Debug config. No separate compiler is needed. One wrinkle: the ASan runtime DLL (`clang_rt.asan*dynamic-x86_64.dll`) must sit next to the `.exe`, and MSVC only auto-copies it through Visual Studio's own `EnableASan` project property, which plain `target_compile_options(/fsanitize=address)` does not set. So a hand-rolled CMake ASan target needs a `POST_BUILD` step to copy that DLL itself. MSVC does not ship UBSan/TSan/MSan; use the GCC/Clang path for those.

**Do not reach for MinGW/MSYS2 g++ on Windows for ASan:** MinGW-w64 g++ does not ship `libasan`, so `-fsanitize=address` compiles but fails to link (`cannot find -lasan`). Getting ASan via VS's bundled Clang needs a non-default Ninja + plain `clang++` setup, because MSBuild links via `lld-link.exe` and silently drops `-fsanitize=address`. The MSVC-native path above is strictly less setup for the same result.

## CMake wiring

A per-target option keeps the sanitizer opt-in and portable across compilers. Pattern (mirrors a working single-target ASan setup):

```cmake
option(ENABLE_ASAN "Build with AddressSanitizer" ON)

if(ENABLE_ASAN)
  if(MSVC)
    target_compile_options(mytarget PRIVATE /fsanitize=address)
    # MSVC needs the ASan runtime DLL beside the exe; VS only copies it via its
    # EnableASan property, which target_compile_options doesn't trigger. Copy it manually.
    add_custom_command(TARGET mytarget POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE_DIR:mytarget>/clang_rt.asan_dynamic-x86_64.dll"  # resolve actual path from the MSVC toolchain
        "$<TARGET_FILE_DIR:mytarget>")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mytarget PRIVATE -fsanitize=address -g -fno-omit-frame-pointer)
    target_link_options(mytarget PRIVATE -fsanitize=address)
  endif()
endif()
```

On GNU/Clang the option can default on with no extra flags since `libasan` links automatically. Note MSVC needs the flag on compile only; GCC/Clang need it on **both** compile and link, hence `target_link_options`.

## CI wiring

Run sanitized builds as separate CI jobs so a normal build stays fast. A practical matrix: a plain Windows/MSVC build job, a Windows/MSVC-native ASan job, and a Linux/g++ ASan job, the sanitized jobs building and running the target so ASan actually exercises the code. Set `ASAN_OPTIONS=detect_leaks=1` on Linux to enable LSan, and treat a non-zero sanitizer exit as a build failure. Example step:
```yaml
- run: cmake -B build -DENABLE_ASAN=ON && cmake --build build && ctest --test-dir build --output-on-failure
```

## Runtime options worth knowing

- `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1`
- `UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1`
- `TSAN_OPTIONS=halt_on_error=1`
- `ASAN_OPTIONS=detect_stack_use_after_return=1` to catch returned-local dangling.

## Valgrind (Linux, no rebuild needed)

`valgrind --leak-check=full --track-origins=yes ./app` catches leaks and invalid accesses on an ordinary (unsanitized) binary. Slower than ASan and misses stack-use-after-return, but needs no recompile and its Memcheck complements ASan. Prefer ASan when you can rebuild; reach for Valgrind on a binary you cannot instrument.
