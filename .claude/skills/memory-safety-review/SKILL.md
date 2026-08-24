---
name: memory-safety-review
description: Deep memory-safety and undefined-behavior analysis of C++, plus sanitizer and static-analysis tooling. Use this skill when the concern is a memory or lifetime bug rather than general code quality: leaks, use-after-free, dangling references, buffer overflow, use-after-move, uninitialized reads, iterator invalidation, or data races and deadlocks; when a sanitizer (ASan, UBSan, TSan, MSan) or Valgrind is firing, should be run, or needs interpreting; or when configuring these in CMake or CI. Trigger on phrases like "why does this leak", "ASan is complaining", "is this a use-after-free", or "data race". For general C++ review (idiom, const-correctness, performance, style), use cpp-code-review instead.
allowed-tools: Read, Grep, Glob, Bash, Edit
---

# Memory Safety Review

A focused pass on memory correctness, lifetime, and undefined behavior in C++, backed by sanitizers and static analysis. This is the specialist. For broad review (idiom, style, performance, interface design) use `cpp-code-review`; invoke this skill when the specific concern is a safety or lifetime bug, or when a sanitizer is involved.

## Step 1: Frame the investigation

Determine which of two modes you are in, because they need different approaches:

1. **Static review** — reading code to find latent safety bugs before they manifest. Work through `references/hazards.md` category by category.
2. **Dynamic diagnosis** — a sanitizer or Valgrind is already firing, or a crash needs reproducing. Go to `references/sanitizers.md` to run the right tool and `references/reading-reports.md` to interpret its output.

Also establish the language standard and the target platform/compiler, since sanitizer availability and invocation differ across MSVC, GCC, and Clang (see `references/sanitizers.md`).

## Step 2: Hunt by category

Do not scan randomly. Walk the hazard categories in `references/hazards.md`, which cover:

- Lifetime and dangling (references/pointers outliving their target, returning locals, dangling `string_view`/`span`).
- Ownership errors (double-free, use-after-free, leaks, `delete` vs `delete[]` mismatch, ownership lost through `.get()`).
- Use-after-move and moved-from misuse.
- Uninitialized reads and partially-constructed objects.
- Iterator/reference invalidation across container mutation.
- Buffer and bounds errors (overflow, off-by-one, out-of-range indexing).
- Undefined behavior beyond memory (signed overflow, invalid casts, strict-aliasing violations, unsequenced modifications).
- Concurrency hazards (data races, torn reads, deadlock, `std::atomic` misuse) — see `references/concurrency.md`.

For each finding: name the hazard, explain the exact sequence that triggers it, state whether it is UB or merely a leak, and give the minimal fix. Prefer fixes that make the bug unrepresentable (RAII, ownership in the type) over fixes that merely patch the symptom.

## Step 3: Confirm dynamically when possible

A static finding is a hypothesis. Where feasible, confirm it by building under the appropriate sanitizer and running the triggering path. `references/sanitizers.md` covers building with ASan (with LeakSanitizer), UBSan, TSan, and MSan, including the MSVC-native and g++/Clang recipes, plus CMake and CI wiring. `references/reading-reports.md` covers turning a sanitizer trace into a root cause: reading the allocation/free/access stacks, distinguishing heap-use-after-free from stack-use-after-return, and dealing with symbolization and false-positive-looking cases.

Match the sanitizer to the bug: ASan for spatial/temporal heap and stack errors and leaks, UBSan for undefined behavior, TSan for races and deadlocks, MSan for uninitialized reads. ASan and TSan cannot run in the same build; note this when recommending both.

## Step 4: Static analysis as a second net

Sanitizers only catch bugs on paths you actually execute. Static analyzers catch some that never ran. `references/static-analysis.md` covers clang-tidy's safety-relevant check groups (`bugprone-*`, `cppcoreguidelines-*`, `clang-analyzer-*`), MSVC `/analyze`, and compiler warning flags worth treating as errors (`-Wall -Wextra -Wshadow` etc.). Recommend these to close the gap the dynamic tools leave open.

## Reference files

Load these as needed; do not read them all up front.

- `references/hazards.md` — The hazard catalog: every memory/UB category with detection cues and fixes.
- `references/sanitizers.md` — Building and running ASan/UBSan/TSan/MSan and Valgrind across MSVC, GCC, and Clang; CMake and CI wiring.
- `references/reading-reports.md` — Interpreting sanitizer output and turning a trace into a root cause.
- `references/concurrency.md` — Data races, deadlock, atomics, and memory-ordering hazards.
- `references/static-analysis.md` — clang-tidy, MSVC /analyze, and warning-flag configuration for catching what sanitizers miss.
