# Reading Sanitizer Reports

A sanitizer trace is a crime scene. The skill is reading the stacks in the right order to get from symptom to root cause.

## Anatomy of an ASan report

An ASan error names the bug type on the first line, then gives up to three stacks:

1. **The access** — where the bad read/write happened (top of the report).
2. **The allocation** — where the memory was allocated ("allocated by thread ... here").
3. **The free/deallocation** — where it was freed ("freed by thread ... here"), present for use-after-free and double-free.

Read them together. For a heap-use-after-free: the access stack is where you touched it, the free stack is where it died, the allocation stack is where it was born. The bug is almost always that the free happens before the access on a path you did not expect. Match the object identity across all three stacks (same address, same allocation site).

## Common ASan error lines and what they mean

- **`heap-use-after-free`** — read/write to freed heap. Look at the free stack: who freed it, and why did the access outlive that? Usually a raw pointer or `.get()` result surviving a `reset()`/`delete`/container erase.
- **`heap-buffer-overflow`** / **`stack-buffer-overflow`** — out-of-bounds by N bytes. The report says read/write and the offset past the region. Usually off-by-one or a wrong size. `global-buffer-overflow` is the same on a static array.
- **`stack-use-after-return`** — a pointer/reference to a local used after the function returned. Needs `detect_stack_use_after_return=1` (default on in recent toolchains). Points straight at a returned/escaped local.
- **`heap-use-after-scope`** — use of a local after its enclosing scope ended.
- **`double-free`** or **`attempting free on address which was not malloc`** — freed twice, or freed something not heap-owned. Two owners, or a `delete` on a smart-pointer-owned or stack object.
- **`detected memory leaks`** (LSan) — allocation stack shows where the never-freed block was born; the owner on that path never released it.

## UBSan output

UBSan prints the source location and the specific UB kind: `signed integer overflow`, `load of misaligned address`, `member call on null pointer`, `load of value which is not a valid value for type` (e.g. a bad `enum`/`bool`). It reports and continues by default; add `-fno-sanitize-recover=all` to abort at the first hit so it cannot cascade. Each report is a single point; no allocation/free stacks.

## TSan output

A data-race report shows two stacks: the two conflicting accesses (at least one a write) to the same address from different threads, plus where each thread was created and any locks held. The fix is to establish a happens-before relationship (mutex, atomic) between the two accesses, or to prove they cannot overlap. See `references/concurrency.md`.

## Turning a trace into a fix

1. Identify the bug class from the first line.
2. Follow the stacks to the exact object and the exact conflicting operations.
3. State the precise interleaving or path that triggers it in one sentence.
4. Fix at the ownership/lifetime level, not by silencing the tool.
5. Rebuild sanitized and rerun the same path to confirm the report is gone.

## When a report looks like a false positive

Genuine ASan/TSan false positives are rare; suspect your code first. Real causes of confusing reports: missing symbolization (rebuild with `-g`; set `ASAN_SYMBOLIZER_PATH` if stacks show addresses instead of names), an uninstrumented library on the boundary (a leak or race inside third-party code you did not build with the sanitizer), or interceptor limits. If you must suppress a known-benign report, use a suppression file (`LSAN_OPTIONS=suppressions=...`, `TSAN_OPTIONS=suppressions=...`) and document why, rather than disabling the sanitizer wholesale.
