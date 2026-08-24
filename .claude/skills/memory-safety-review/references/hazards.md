# Hazard Catalog

Walk these categories in order when statically reviewing for safety. For each hazard: the cue that reveals it, and the fix.

## 1. Lifetime & dangling

The target is destroyed while a reference or pointer to it still lives.

- **Returning a reference/pointer to a local.** `int& f() { int x = 0; return x; }` returns a dangling reference. Cue: a non-static local's address or reference escaping the function. Fix: return by value, or return a reference to something that outlives the call.
- **Dangling `string_view` / `span`.** These are non-owning views. Storing one past the lifetime of the buffer it views is a dangling read. Cue: a `string_view`/`span` member, or one returned from a function whose backing store is local or temporary. Common trap: `std::string_view sv = std::string{"x"} + "y";` views a destroyed temporary. Fix: own the data (`std::string`), or guarantee the backing store outlives the view.
- **Reference/pointer members outliving their target.** A stored `T&` or `T*` whose referent is destroyed first. Cue: a class holds a reference/raw pointer it did not create. Fix: document and enforce the lifetime relationship, or switch to shared ownership / `weak_ptr` if it cannot be guaranteed.
- **Lambda capture by reference outliving the scope.** A lambda `[&]` stored or run later reads dead locals. Cue: `[&]` on a lambda that escapes (stored in a container, passed to async work, returned). Fix: capture by value, or capture the specific long-lived objects.

## 2. Ownership errors

- **Use-after-free.** Access through a pointer whose target was freed. Cue: a raw pointer surviving past a `delete`, `reset()`, container erase, or reallocation. Fix: express ownership in the type (`unique_ptr`), null the pointer, or restructure so the access cannot follow the free.
- **Double-free.** Same resource freed twice. Cue: two owners each calling `delete`, or a raw `delete` on a pointer a smart pointer also owns. Fix: single clear owner; never `delete` a pointer a smart pointer manages.
- **Leak.** Owner never frees. Cue: raw `new` with no matching `delete` on every path (including exceptions and early returns), or a broken ownership handoff. Fix: RAII; never hand-manage lifetime.
- **`delete` vs `delete[]` mismatch.** Array `new[]` freed with scalar `delete` (or vice versa) is UB. Fix: prefer `std::vector` / `std::unique_ptr<T[]>`; avoid raw arrays.
- **Ownership lost through `.get()`.** Passing `smart.get()` into something that stores and later frees it creates two owners. Cue: `.get()` feeding a sink that takes ownership. Fix: pass the smart pointer (move for `unique_ptr`), or make the sink non-owning.

## 3. Use-after-move

Reading a moved-from object beyond a reset. A moved-from object is valid but unspecified: you may assign to it or destroy it, not rely on its value.

- Cue: a variable read after appearing as the argument to `std::move`, or after being the source of a move construction/assignment.
- Fix: do not read moved-from state; reassign before use, or restructure so the move is the last use.

## 4. Uninitialized reads

Reading a value never written.

- Cue: a primitive member with no in-class initializer and no assignment in every constructor path; a local `int x;` read before assignment; a `struct` created without `{}` leaving members indeterminate.
- Fix: in-class default initializers (`int m_Count{};`), constructor init lists covering every member, or brace-init `{}` for zero-init. Treat `-Wuninitialized` / MSan hits as real.

## 5. Iterator & reference invalidation

Mutating a container invalidates iterators, pointers, and references into it, per container rules.

- **Reallocation.** `push_back`/`insert`/`resize`/`reserve` on `vector` may reallocate, invalidating everything pointing into it. This is the classic "dangling reference into a `std::vector` after it grows" drill: hold a reference to `v[0]`, `push_back` past capacity, then read the stale reference.
- **Erase.** Erasing invalidates iterators at and after the point (container-dependent); using the old iterator afterward is UB. Fix: use the returned iterator (`it = v.erase(it);`).
- Cue: a saved iterator/reference/pointer into a container that is mutated before the saved handle is used again. Fix: re-acquire after mutation, reserve up front, or index by position where the container permits.

## 6. Buffer & bounds

- Off-by-one in loop bounds; `<=` where `<` was meant.
- `operator[]` past the end (no bounds check; UB). `.at()` throws instead but costs a check.
- C arrays and `memcpy`/`strcpy` with wrong sizes.
- Fix: prefer range-for and algorithms over index arithmetic; `std::span` carries a size; `.at()` or explicit checks at trust boundaries.

## 7. Undefined behavior beyond memory

- **Signed integer overflow** is UB (unsigned wraps). Cue: unchecked arithmetic near `INT_MAX`. Fix: check, use wider types, or `<numeric>` saturating ops (C++26) / manual guards.
- **Invalid casts.** `reinterpret_cast` then dereference at the wrong type; `static_cast` down a hierarchy the object is not. Fix: `dynamic_cast` for polymorphic downcasts (check the result), avoid type punning via casts.
- **Strict-aliasing violations.** Reading an object through an incompatible pointer type. Fix: `std::bit_cast` (C++20) or `memcpy` for type punning, not pointer casts.
- **Unsequenced modification.** `i = i++ + ++i;` and friends. Fix: split into sequenced statements.

## 8. Concurrency

See `references/concurrency.md` for data races, torn reads, deadlock, and atomics. These are memory-safety bugs too: a data race on a non-atomic is UB regardless of whether it appears to "work."
