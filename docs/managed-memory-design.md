# Managed memory design

Status: first implementation in place; ABI 1 remains experimental. Windows
x86/x64 and Linux x64 implementations have been built and validated. Linux
static and shared builds pass the native runtime test suite. Both platforms
preserve the one-OS-thread-per-process contract.

## Collector

Use a non-moving, conservative mark-and-sweep collector for blocks obtained from
`Memory.Alloc`. Centralizing allocation in `Memory` lets the runtime track
objects, arrays, array backing stores, strings and their buffers, and nullable
value storage without changing their layouts. The collector stores allocation
records separately from managed blocks, so the existing `Object._gc` field
remains reserved and object/interface ABI layouts do not change.

The collector treats aligned pointer-sized words in a block as possible
references. It accepts interior pointers and marks the containing allocation.
This naturally traces class fields, array contents, interface references,
nullable payloads, and cycles without per-type descriptors. False positives can
delay reclamation; moving objects and finalizers are out of scope.

## Roots and modules

At a collection point, scan the active native stack and a saved register context
for the current thread. Generated modules register the address and size of each
static field that can retain a managed pointer. Native C code that stores a
managed pointer in static storage must register that storage through the runtime
root API. Synchronous native calls on the CX thread are safe because their live
stack frames remain on the scanned stack. A managed pointer retained beyond a
native call must be kept in a registered static root or another managed block.

All CX modules in one process must use the same `cxcore` heap. Project-reference
static linking already links one runtime into an executable; dynamic modules
must share one `cxcore` DLL. Dynamically unloading a module that registered roots
is unsupported in the first collector. Generated code registers roots before
executing CX functions, including in library-only modules.

## Threading and collection points

The first collector supports one OS thread per process. The first managed-heap
operation claims the collector thread. Allocation, reallocation, freeing, root
registration, and collection from another thread fail with a diagnostic. This
avoids unsafe collection while another stack or the heap is changing. CX code
may call native C synchronously on the collector thread; native code must not
transfer managed pointers to other threads. Multi-threaded CX and foreign-thread
callbacks require a later design with thread registration, safepoints, and
stop-the-world coordination.

Collection occurs under allocation pressure and can be requested explicitly
through the C runtime API for embedding and native diagnostics. The initial
threshold is 1 MiB and grows to at least twice the live managed bytes after a
collection. Allocation retries after collection before reporting out-of-memory.

## Explicit memory APIs and failures

Every successful `Memory.Alloc` block is tracked and collectible. `Memory.Free`
removes a tracked block and releases it immediately; freeing a block while any
live reference can be used is invalid. `Memory.Realloc` preserves the block's
tracking record but, as with C `realloc`, any pointer invalidated by a move must
not be used. Runtime-owned allocations continue to use `Memory.Alloc`; callers
must not add per-type disposal requirements. `Memory.Free` remains available
for deterministic cleanup and unmanaged interop, with its use on live managed
objects subject to the invalid-use rule above.

Allocation metadata uses the process C allocator directly and is never traced.
If metadata or collection work cannot be allocated, the runtime retains all
managed blocks and reports out-of-memory if the requested allocation still
cannot be satisfied. The collector never moves or finalizes blocks.

## Validation

The native test suite covers stack and registered static roots, object graphs,
arrays and backing stores, nullable payloads, interface references, cycles,
allocation pressure, references retained across the cxcore DLL boundary, and
rejection of a second thread. The suite and Gradebook integration example have
been built and run for Windows x86/x64; the full suite also passes for Linux
static and shared builds. Manual `Free` and `Realloc` tracking are exercised
through runtime API checks. The implementation remains experimental:
conservative scanning can retain unreachable blocks, native static roots must
be registered, and modules with registered roots cannot be unloaded.
