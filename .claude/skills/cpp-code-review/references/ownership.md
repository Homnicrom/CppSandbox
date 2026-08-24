# Ownership & Memory

The core question for any C++ review: **who owns each resource, and is that ownership expressed in the type?**

## Smart pointer selection

| Situation | Use |
| --- | --- |
| Single, exclusive owner | `std::unique_ptr<T>` |
| Shared ownership, lifetime tied to multiple holders | `std::shared_ptr<T>` |
| Observe without owning; may outlive the object | `std::weak_ptr<T>` (lock before use) |
| Non-owning, always-valid reference | raw `T*` or `T&` (never `delete` it) |

Default to `unique_ptr`. Reach for `shared_ptr` only when ownership is genuinely shared; it carries atomic refcount overhead and obscures lifetime. A `shared_ptr` passed where the callee only observes should be a raw pointer or reference.

## Ownership in the type system

Prefer to make illegal states unrepresentable. If a function borrows, take `T&` or `const T&`. If it takes ownership, take `unique_ptr<T>` by value. If it shares, take `shared_ptr<T>`. The signature should tell the caller what happens to the resource without reading the body.

```cpp
void Observe(const Widget& w);           // borrows, no ownership
void Consume(std::unique_ptr<Widget> w); // takes ownership
void Share(std::shared_ptr<Widget> w);   // shares ownership
```

## Custom deleters and factory control

A custom deleter lets a type control its own destruction while still living in a smart pointer. This is the pattern to reach for when construction/destruction must go through controlled paths (e.g. a private `delete this`, a pool, or an allocator).

```cpp
struct MyDeleter {
    void operator()(T* p) const { if (p) p->Destroy(); }
};
std::unique_ptr<T, MyDeleter> Make() {
    std::unique_ptr<T, MyDeleter> p{ new T, MyDeleter{} };
    p->Init();
    return p;
}
```

When reviewing such a factory, confirm: the destructor path is actually reached, the deleter handles null, and a matching `shared_ptr` overload (if present) wires the same deleter into its control block.

## weak_ptr and cycles

Two objects holding `shared_ptr` to each other never reach refcount zero and leak. Break the cycle by making the non-owning direction a `weak_ptr`. A registry or observer list that must not extend the lifetime of its entries is the classic case: store `weak_ptr`, and `lock()` at point of use, skipping entries that have expired.

```cpp
std::vector<std::weak_ptr<IThing>> m_Observers;
for (const auto& wp : m_Observers)
    if (auto sp = wp.lock())
        sp->Notify();
```

When reviewing weak_ptr usage, check that every `lock()` result is tested before dereference, and that expired entries are eventually pruned so the container does not grow unbounded.

## Rule of Zero / Three / Five

- **Rule of Zero:** if a class manages no raw resource, declare none of the special members. Let the compiler generate them. This is the target.
- If you declare a destructor, copy constructor, or copy assignment, you almost certainly need all of the relevant five (dtor, copy ctor, copy assign, move ctor, move assign). Flag partial declarations.
- A polymorphic base class needs a `virtual` destructor (or a protected non-virtual one if deletion through base is disallowed).

## Common flags

- Raw `new`/`delete` outside a deleter or RAII wrapper.
- Returning a reference or pointer to a local.
- Storing a reference/pointer whose target may outlive or predecease the holder.
- `.get()` on a smart pointer passed into something that stores it (ownership silently lost).
- Use-after-move: reading a moved-from object beyond a valid-but-unspecified reset.
