# Managed memory design

The runtime uses a non-moving conservative collector with cooperative
stop-the-world coordination. The generated-code ABI remains experimental;
see [runtime-abi.md](runtime-abi.md) for compatibility requirements.

## Collector

The non-moving conservative mark-and-sweep collector manages blocks obtained from
`Memory.Alloc`. Centralizing allocation in `Memory` lets the runtime track
objects, arrays, array backing stores, strings and their buffers, and nullable
value storage without changing their layouts. The collector stores allocation
records separately from managed blocks, so the existing `Object._box` field
remains available for box metadata while ordinary objects leave it null; object
and interface ABI layouts do not change.

The collector treats aligned pointer-sized words in a block as possible
references. It accepts interior pointers and marks the containing allocation.
This naturally traces class fields, array contents, interface references,
nullable payloads, and cycles without per-type descriptors. False positives can
delay reclamation; moving objects and finalizers are out of scope.

## Roots and modules

At a collection point, scan the collector's active native stack and saved register
context plus those of every parked, registered thread. Generated modules register the address and size of each
static field that can retain a managed pointer. Native C code that stores a
managed pointer in static storage must register that storage through the runtime
root API. Synchronous native calls on the CX thread are safe because their live
stack frames remain on the scanned stack. A managed pointer retained beyond a
native call must be kept in a registered static root or another managed block.

All CX modules in one process must use the same `cxcore` heap. Project-reference
static linking already links one runtime into an executable; dynamic modules
must share one `cxcore` DLL. Dynamically unloading a module that registered roots
is unsupported. Generated code registers roots before
executing CX functions, including in library-only modules.

## Threading and collection points

The first managed-heap operation implicitly attaches the startup thread. Native
workers explicitly attach before managed operations and detach before exit.
Unregistered threads fail with a diagnostic. Heap bookkeeping and static root
registration are serialized; collection waits for every other attached thread
to park at a heap operation or `cx_gc_safepoint`. Allocation-free native loops
must poll explicitly. Native-only blocking waits use `cx_gc_run_blocking`, whose
active frame keeps the caller's stack and registers available to the collector.
The callback must return normally and must not access managed data or enter CX.
Application data sharing still requires synchronization. Generated CX workers
poll at function entry and supported loops. Module initialization, pending TLS
exceptions, and Console blocking waits are coordinated with collection.
Native callback scopes and removable root handles establish retention ownership.
Reentrant imports use `cx_gc_run_native_call`, which allows guarded callbacks to
resume managed execution. The native-only `cx_gc_run_blocking` wrapper still
forbids CX reentry; see [native-interop-design.md](native-interop-design.md).

Collection occurs under allocation pressure and can be requested explicitly
through the C runtime API for embedding and native diagnostics. The initial
threshold is 1 MiB and grows to at least twice the live managed bytes after a
collection. Allocation retries after collection before reporting out-of-memory.

## Explicit memory APIs and failures

Every successful `Memory.Alloc` block is tracked and collectible. `Memory.Free`
removes a tracked block and releases it immediately; freeing a block while any
live reference can be used is invalid. `Memory.Realloc` preserves the block's
tracking record but, as with C `realloc`, any pointer invalidated by a move must
not be used. `Memory.Free` rejects arena storage. `Memory.Realloc` accepts an
exact live allocation in the active owning arena, allocates and copies within
that arena, and invalidates the old extent; its bytes remain reserved until
reset. `Memory.Free` preserves its native-free behavior for untracked non-arena
pointers; callers must pass a pointer compatible with the process C allocator.
Runtime-owned allocations
continue to use `Memory.Alloc`; callers must not add per-type disposal
requirements. Native resources and unmanaged buffers use their dedicated APIs
or native allocator.

Allocation metadata uses the process C allocator directly and is never traced.
If metadata or collection work cannot be allocated, the runtime retains all
managed blocks and reports out-of-memory if the requested allocation still
cannot be satisfied. The collector never moves or finalizes blocks.

## Arena allocation

`Memory.Alloc` routes to the active arena context when one exists. An explicit
durable context keeps new allocations on the ordinary GC heap. Owner-aware
buffers continue to follow their existing owner's allocation destination.

The collector scans initialized ranges in active and logically retained arenas,
keeping reachable ordinary-heap objects alive. Arena storage is reclaimed by
scope closure or reset, rather than by tracing individual arena objects.

`Memory.WithArena(action)` creates a synchronous growable scope;
`Memory.WithArena(capacity, action)` creates a fixed-capacity scope.
`Memory.WithDurable(action)` temporarily selects the GC heap. Each restores
the previous allocation context on normal and exceptional exit. Reusable and
asynchronous arenas have separate lifetime rules.

Runtime-once initialization uses durable storage for process-lifetime metadata.
Compiler provenance checks and runtime store barriers reject supported escape
paths, but do not prove every unsafe aggregate or indirect store safe. Raw
copies scan candidate references conservatively; typed copies use reflected
layout checks. See [scoped allocation arenas](scoped-allocation-arenas-design.md)
for allocation routing, reference lifetimes, process budgets, and copy limits.

## Limits

Conservative scanning can retain unreachable blocks. Native static storage
holding managed references must be registered, and modules with registered
roots cannot be unloaded. Native workers must follow the attachment,
safepoint, and blocking-call contracts in
[threading and GC coordination](threading-gc-design.md).

The collector does not move or finalize blocks. Dispose native resources
explicitly; collecting their managed wrappers is not a cleanup protocol.
