# Scoped allocation arenas

Allocation arenas group temporary managed allocations under an explicit
lifetime. Allocations made in an arena, including those made by called
functions, are released or reset together when the scope closes. Outside an
arena, ordinary allocations use the managed GC heap.

Arenas are experimental. Compiler lifetime checks and runtime store barriers
cover supported paths, but do not establish memory safety for arbitrary unsafe
code, foreign retention, or unmodeled indirect stores.

## Public APIs

| API | Behavior |
| --- | --- |
| `Memory.WithArena(action)` | Run a synchronous callback in a growable temporary arena, then release its storage |
| `Memory.WithArena(capacity, action)` | Run a synchronous callback in fixed-capacity preallocated storage |
| `MemoryArena(initialCapacity)` | Reserve a reusable growable arena |
| `MemoryArena(initialCapacity, maximumCapacity)` | Reserve a reusable arena with a capacity ceiling |
| `MemoryArena.Run(action)` | Run one synchronous allocation generation, then reset it while retaining chunks |
| `MemoryArena.Dispose()` | Release the reusable arena's reserved storage |
| `Memory.WithDurable(action)` | Temporarily route new managed allocations to the ordinary GC heap |
| `Memory.WithArenaAsync(action)` | Retain an arena across the task returned by the callback |
| `Memory.WithArenaAsync(capacity, action)` | Use a fixed-capacity arena across the returned task |
| `Memory.WithArenaAsync<T>(action)` | Complete a typed result only after the retained scope closes; a capacity overload is also available |

The synchronous callbacks return `void`. Each restores the prior allocation
context on normal and exceptional exit. `WithDurable` is synchronous even when
used inside an asynchronous arena operation.

## Synchronous scopes

```cx
import System;

public void ProcessTemporaryValues() {
    Memory.WithArena(() => {
        int[] scratch = new int[128];
        scratch[0] = 42;
        // Consume scratch before this callback returns.
    });
}
```

References allocated by the callback expire when it returns. Do not return
them, assign them to a longer-lived object, or retain them in a caller's stack
frame. A synchronous scope cannot be extended by starting asynchronous work.

A reusable arena retains reserved chunks between runs:

```cx
import System;

public void RunFrames() {
    MemoryArena arena = new MemoryArena(4096u);
    try {
        arena.Run(() => {
            int[] scratch = new int[64];
            scratch[0] = 1;
        });
        arena.Run(() => {
            int[] scratch = new int[64];
            scratch[0] = 2;
        });
    } finally {
        arena.Dispose();
    }
}
```

Each `Run` starts a new allocation generation. Retaining the same backing
memory does not extend the lifetime of an earlier generation. Reset clears
allocation extents and subsequent allocations are zero-initialized.

`Run` is synchronous and non-reentrant. A nested or overlapping `Run` is
rejected before the second callback executes. Running a disposed arena or
disposing an active arena throws. `Dispose` is idempotent; arenas have no
finalizer and must be disposed explicitly.

## Capacity and allocation

Arena allocations are aligned and zero-initialized. Growable arenas reserve
additional chunks as required. Fixed-capacity arenas reserve their chunk before
the callback and throw a catchable CX exception on exhaustion; they do not
fall back to the GC heap.

The two-argument `MemoryArena` constructor requires a nonzero initial capacity
and a maximum at least as large as that initial capacity. Equal capacities
create a fixed arena; a larger maximum permits bounded growth. After an
allocation failure, `Run` still resets the arena so it can be reused.

Alignment padding and allocation-ownership metadata consume chunk capacity.
The capacity is therefore not simply the sum of requested payload sizes.
Control records and chunk descriptors are separate setup overhead.

Owner-aware allocations follow their existing owner. For example, growing a
durable list while an arena is active keeps its replacement buffer on the GC
heap. Growing an arena-owned collection follows that arena even inside
`WithDurable`. Durable construction affects newly created owned graphs; it
does not change the lifetime of an existing object.

## Reference lifetimes

Each allocation has a region and a generation. Checked stores validate the
source lifetime against the destination:

- An arena may refer to durable objects. The collector scans active or logically
  retained arena allocations so those durable objects remain reachable.
- A nested arena may borrow from an ancestor that outlives every use. An
  ancestor cannot retain a child reference after the child scope closes.
- Independent arenas cannot directly retain one another's references.
- Durable objects, statics, caller-frame stack slots, and unretained callbacks
  cannot retain arena references.
- Interior pointers into a live allocation are recognized. Pointers into
  padding, inactive arenas, or stale allocation extents are invalid.

These rules apply to objects, arrays, collection buffers, value structs,
interfaces, boxes, nullable payloads, delegates, tasks, and native retention
paths. A shallow copy does not make nested references durable.

The compiler tracks scoped provenance through supported assignments, calls,
collection operations, and captures. An unanalyzable call requires a trusted
borrow contract or a durable copy. `System.Interop.NoEscape` promises that a
call does not retain its inputs; it does not make returned references durable.
Virtual and interface calls require the contract on the dispatch slot, and
known implementations are still checked.

## Durable results and graph copies

Use `Memory.WithDurable` to construct results that must survive a scope. Create
or copy the entire owned result graph in that callback. Runtime barriers still
reject a durable object that points back into temporary arena storage.

The [arena scopes example](../examples/arena-scopes) demonstrates scalar and array
copies and a type-specific graph clone. `CloneContext` provides an identity-based
source-to-copy map so a graph copier can preserve aliases and cycles. Its
default limits are 65,536 copied objects and a depth of 1,024; the constructor
can specify smaller or larger positive bounds.

Register a copy before following graph links when the copy policy permits
cycles. Constructor-bound cycles are rejected when construction cannot register
an object first. Native resources, delegates, shared references, and other
identity-sensitive values require explicit copy policies. `CloneContext` is a
helper for a copier, rather than an automatic copier for every object shape.
Keep its source-to-copy map temporary; only the copied result should outlive
the operation.

## Asynchronous scopes

`WithArenaAsync` retains the arena until the callback's returned task completes.
The runtime carries logical arena ownership through supported task
continuations, scheduler callbacks and timers, cancellation callbacks, threads,
reusable task sources, and native callback registrations. A multicast action is
rejected.

A retained arena is still serialized. Logical entry waits while another thread
has that arena active; this does not permit concurrent mutation of arena data.
A callback may retain references from its own retained arena and durable
storage, but not from an unrelated arena.

Waits that would prevent reentry are rejected. In particular, joining a worker
from the same still-active arena or unregistering a callback that needs to
reenter that arena can deadlock and is not allowed. Native registrations retain
their lease until unregistration and all in-flight callbacks finish.

The generic overload checks that the callback returns the expected closed
`Task<T>`. It copies the result through typed lifetime barriers before closure
and settles the returned task after closure. A result that still references
the closing arena is rejected. Construct a durable result when a reference
must cross that boundary.

Only the task returned by the callback defines the join boundary. Arbitrary
detached work and unrelated task graphs do not gain ownership merely because
they started in an arena. Cancellation requests do not release storage while
native consumers or retained callbacks can still use it.

## Exceptions and resource cleanup

Normal completion, early return, and CX exceptions restore allocation contexts
and close each scope once. An exception thrown from scoped storage is copied
to durable storage before unwinding. The copier preserves supported derived
types with scalar or durable payload fields; unsupported or scoped extra fields
cause a safe base-`Exception` copy.

Arena release does not run finalizers or replace resource disposal. Release
native buffers and other external resources in `finally` before their managed
owners become invalid. A retained async operation must finish its cleanup
before publishing completion.

## Raw memory operations

`Memory.Free`, `Memory.Realloc`, `Memory.Copy`, and `Memory.CopyValue<T>`
require unsafe code.

`Free` rejects arena storage, including unused chunk capacity. `Realloc`
accepts only an exact live allocation in the active owning arena. It allocates
and copies within that arena, invalidates the old extent, and retains the old
reserved bytes until reset. A zero-size resize invalidates the extent without
reclaiming its reserved bytes. Old aliases become invalid after resizing.

Raw `Memory.Copy` rejects ranges intersecting arena chunks and scans candidate
reference windows for lifetime violations. Pointer-shaped scalar bytes can be
rejected conservatively. `Memory.CopyValue<T>` copies one struct through
reflected layout checks; source and destination must cover `sizeof(T)` bytes,
must not partially overlap, and must have a supported runtime layout.

Native aggregates with complete projected layouts use checked pointer offsets.
Incomplete projections are limited to conservative stack copies and cannot be
published into longer-lived storage. Unresolved erased generic layouts are
not a general typed-copy path.

## Combined managed-storage budget

`Memory.SetProcessMemoryBudget` and `cx_gc_set_process_memory_budget` set one
process-wide limit for tracked managed-heap payload bytes plus reserved live
arena chunk capacities. Zero disables the limit.

The setter returns false without changing the limit if the requested value
cannot fit `size_t` or current usage already exceeds it. Heap growth may collect
before failing with `Out of memory`; arena reservation fails directly. Reset
retains its reservation, while destruction releases it.

The budget excludes thread stacks, runtime tables, native resources, arena
control records, and chunk descriptors. It is a managed-storage limit, rather
than a resident-set or whole-process limit.

## Limits and related documentation

Conservative collection and store scanning can retain objects or reject
pointer-shaped scalar data. Unsafe pointer validity, unknown native retention,
unmodeled aggregate stores, and arbitrary asynchronous ownership are outside a
general safety guarantee. Never use a rejected escape by bypassing its checks.

See [managed memory](managed-memory-design.md),
[threading and GC coordination](threading-gc-design.md), and
[native interoperability](native-interop-design.md) for the surrounding
contracts. Build and measurement instructions are in the
[README](../README.md) and [benchmark guide](../benchmarks/README.md).
