# Pre-flight Checklist

Run this over any header/source pair before writing the review. It is a scan, not a script; skip items that do not apply.

## Headers

- [ ] `#pragma once` (or include guard) present.
- [ ] Includes minimal; forward declarations used where a full definition is not needed.
- [ ] No `using namespace` at file scope in a header.
- [ ] Public interface separated cleanly from private members.

## Types & special members

- [ ] Polymorphic base has a `virtual` (or protected) destructor.
- [ ] Rule of Zero honored; if not, all five special members accounted for.
- [ ] Single-argument constructors marked `explicit` unless implicit conversion is intended.
- [ ] Members initialized (in-class default `{}` or constructor init list); no uninitialized primitives.
- [ ] Copy/move semantics correct for what the class owns.

## Const & interface quality

- [ ] Non-mutating methods are `const`.
- [ ] Non-trivial parameters passed by `const&` (or by value + move when ownership transfers).
- [ ] `[[nodiscard]]` on functions whose return must not be ignored.
- [ ] `noexcept` on move operations and non-throwing functions.

## Ownership & memory

- [ ] Every resource has one clear owner, expressed in the type.
- [ ] No raw `new`/`delete` outside a deleter or RAII wrapper.
- [ ] `shared_ptr` justified (not where `unique_ptr` fits).
- [ ] `weak_ptr` used where a holder must not extend lifetime; every `lock()` is null-checked.
- [ ] No dangling references/pointers; nothing returns a reference to a local.
- [ ] No use-after-move.

## Correctness

- [ ] No iterator invalidation across container mutation.
- [ ] `override` on every overriding method; signatures actually match the base.
- [ ] Bounds and null checks where indices or pointers can be invalid.
- [ ] Integer/enum conversions intentional.

## Idiom & performance (for the detected standard)

- [ ] Algorithms/ranges preferred over hand-rolled loops where clearer.
- [ ] Unnecessary copies and allocations removed; moves and `reserve()` used where they matter.
- [ ] Modern facilities (`std::print`, `enum class`, concepts, `optional`/`expected`) used where the standard allows.

## Output discipline

- [ ] Findings ordered by severity: blockers, then improvements, then nitpicks.
- [ ] Each blocker has problem, impact, and concrete fix.
- [ ] Fixes shown as minimal snippets with reasoning, not blanket rewrites.
