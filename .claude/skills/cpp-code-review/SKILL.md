---
name: cpp-code-review
description: Review, write, and refactor modern C++ (C++17/20/23) with an emphasis on correctness, memory safety, RAII, and performance. Use this skill whenever the user asks to review C++ code, write new C++ classes or functions, refactor existing C++, hunt memory or ownership bugs, evaluate a header/source pair, or check code against the project's conventions. Trigger it whenever there are code changes or when prompted to review, check or improve code.
allowed-tools: Read, Grep, Glob, Write, Edit
---

# C++ Code Review

Review and author modern C++ that is correct first, safe second, efficient third, and readable throughout. Match the standard and conventions already present in the project rather than imposing a personal style.

## Step 1: Establish context before commenting

Never review or write C++ in a vacuum. First determine:

1. **Language standard.** Look for `CMAKE_CXX_STANDARD`, a `/std:` flag, or usage signals in the code: `std::print`/`std::println` and `concepts` imply C++20+; `std::variant`, `std::optional`, structured bindings imply C++17+. When in doubt, ask or state the assumption explicitly.
2. **Project conventions.** Infer naming (`m_Member`, `PascalCase` methods, brace-init `{}`), header layout, include style, and error-handling approach from surrounding files. A review that fights the codebase's own style is noise.
3. **Purpose.** Distinguish production code from practice/learning code. For practice code, favor teaching the idiom over demanding production hardening.

If any of these is genuinely ambiguous and affects the review, ask one focused question rather than guessing broadly.

## Step 2: Review priorities, in order

Report findings by severity. Correctness and safety issues are blockers; style is advisory.

1. **Correctness** — Undefined behavior, dangling references, iterator invalidation, use-after-move, uninitialized members, off-by-one, incorrect virtual/override, missing `virtual` destructor on a base used polymorphically.
2. **Memory & ownership** — See `references/ownership.md`. Who owns what, and is it expressed in the type? Raw `new`/`delete` outside a controlled RAII wrapper is a flag. `shared_ptr` where `unique_ptr` suffices is overhead. Cycles that need `weak_ptr`.
3. **Const-correctness & interface** — `const` on non-mutating methods, pass-by-`const&` for non-trivial params, `[[nodiscard]]`, `noexcept` where it holds, `explicit` on single-arg constructors.
4. **Modern idiom** — See `references/modern-cpp.md`. Prefer algorithms over raw loops, `enum class` over plain enum, `auto` where it aids clarity, ranges/views (C++20), concepts over SFINAE (C++20), `std::print` over `iostream` (C++23/`<print>`).
5. **Performance** — Unnecessary copies, allocations in hot paths, missed moves, `reserve()` opportunities, passing large types by value. Only flag when it matters; do not micro-optimize cold code at the cost of clarity.
6. **Style & readability** — Naming consistency, header hygiene (`#pragma once`, minimal includes, forward declarations), and clear structure. Lowest priority.

When the code under review is a test file, also apply the redundancy, unfalsifiability, and name-drift checks in the `googletest` skill: a case that cannot fail or that duplicates another one is a finding, the same as dead production code.

## Step 3: Deliver the review

Structure output as:

- A one-line verdict (e.g. "Sound ownership model; two correctness issues and a few idiom improvements").
- **Blockers** (correctness/safety), each with the problem, why it matters, and a concrete fix.
- **Improvements** (idiom/performance), grouped and prioritized.
- **Nitpicks** (style), kept brief.

Show corrected code in minimal diff-style snippets, not full-file rewrites, unless a rewrite is requested. Explain the reasoning behind each non-obvious fix so the user learns the principle, not just the patch. When you reject or would reject a change, say why plainly.

## Reference files

Load these as needed; do not read them all up front.

- `references/ownership.md` — Smart pointer selection, custom deleters, RAII patterns, ownership-in-the-type-system, weak_ptr cycles.
- `references/modern-cpp.md` — Standard-by-standard idiom cheatsheet (C++17/20/23): concepts, ranges, `std::print`, `std::expected`, algorithms, and common anti-patterns to flag.
- `references/checklist.md` — A fast pre-flight checklist to run over any header/source pair before writing the review.
