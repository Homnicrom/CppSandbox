# Concurrency Hazards

Concurrency bugs are memory-safety bugs: a data race on a non-atomic object is undefined behavior even when the program appears to work. They are also non-deterministic, so static reasoning plus ThreadSanitizer matters more here than casual testing.

## Data races

A data race is two threads accessing the same memory location, at least one writing, with no synchronization establishing a happens-before order between them.

- **Cue:** a shared mutable variable touched from more than one thread without a mutex or atomic guarding every access. A single unguarded write is enough.
- **Trap:** "it's just a bool/int, the write is atomic in practice." Not in the abstract machine; a race on a non-atomic is UB regardless of hardware. The compiler may tear it, hoist it, or assume it never changes.
- **Fix:** guard every access to the shared object with the same mutex, or make it `std::atomic<T>`. Const-only shared access needs no guard; the moment one thread writes, all accesses need synchronization.

## Torn reads / writes

A non-atomic multi-word object read while another thread writes it can yield a value that was never coherently stored (half-old, half-new).

- **Cue:** a wide struct or 64-bit value shared without atomics or a lock.
- **Fix:** `std::atomic` for lock-free single values, or a mutex for anything larger than the platform's lock-free size (`std::atomic<T>::is_lock_free()`).

## Deadlock

Two or more threads each waiting on a lock the other holds.

- **Cue:** nested locking where two mutexes can be acquired in opposite orders on different threads.
- **Fix:** impose a global lock ordering, or acquire multiple locks together with `std::scoped_lock(m1, m2)` (deadlock-avoiding). Prefer holding one lock at a time; minimize critical-section scope.

## Atomics and memory ordering

- `std::atomic<T>` operations default to `memory_order_seq_cst` (sequentially consistent) which is correct but the strongest/slowest ordering. Weaker orderings (`acquire`/`release`, `relaxed`) are an optimization that is easy to get subtly wrong.
- **Cue to flag:** explicit `memory_order_relaxed` or `acquire`/`release` in code that has not clearly established why the weaker guarantee is sufficient. Relaxed atomics do not establish happens-before for *other* data; using them to publish a pointer to non-atomic data is a classic race.
- **Fix / guidance:** default to `seq_cst` unless there is a measured need and a correctness argument for weaker ordering. A release store paired with an acquire load is the minimum to safely publish data written before the store.

## Other hazards

- **Non-atomic `shared_ptr` control-block races.** The refcount is atomic, but the *same* `shared_ptr` instance mutated (reassigned) from two threads is a race; two threads copying from a shared instance while a third reassigns it is a race. Give each thread its own `shared_ptr` copy, or synchronize reassignment.
- **`std::vector`/container shared across threads.** Standard containers are not internally synchronized. Concurrent `push_back` and read is a race and can also invalidate iterators mid-read.
- **Lambda / thread capturing by reference.** A `std::thread` or async task with `[&]` capturing locals that go out of scope before the task runs is both a lifetime bug and a race. Capture by value or `join()` before the scope ends.
- **Missing `join()`/`detach()`.** A `std::thread` destroyed while joinable calls `std::terminate`. Prefer `std::jthread` (C++20, auto-joins) or RAII around the thread.

## Confirming with ThreadSanitizer

TSan (`-fsanitize=thread`, GCC/Clang) instruments memory accesses and reports races with both conflicting stacks. It only flags races on paths that actually interleave during the run, so exercise the concurrent path under load and repeat. TSan cannot share a build with ASan. See `references/reading-reports.md` for reading its output.
