# Threading and GC coordination

## Memory and execution contract

The runtime uses a non-moving conservative collector. A process has one heap
shared by all modules and registered OS threads. One runtime mutex protects
allocation records, static root registration, collection thresholds, and the
thread registry. This mutex does not synchronize application fields: concurrent access to
the same mutable data requires application synchronization. No atomicity is
promised for reference, interface, or aggregate writes. Thread start, join,
mutex release/acquire, and `AtomicInt` operations supply publication
ordering; `volatile` is not a CX synchronization primitive.

## CX API and publication

`System.Threading.Thread`, `Mutex`, and `AtomicInt` are pointer-sized value
handles; copies share managed state. `Thread.Start(delegate() => void)` starts
an attached worker, supports captured/method/multicast function values, and
publishes the action in a registered live-worker list before native startup.
Native handles are detached immediately; successful completion unlinks the
runtime root and unregisters the worker atomically. Handle copies remain valid
as long as reachable. `Join()` waits without preventing collection, publishes
worker writes to the caller, and rethrows the worker's original exception.
Repeated joins are allowed, including repeated failure observation. Unobserved
worker failures stay rooted until observed; normal program exit waits for all
owned workers and surfaces any unobserved failure. There is no asynchronous
thread termination. Unjoined infinite workers keep normal process exit waiting.

`Thread.Id` and `Thread.CurrentId` expose monotonically assigned logical runtime
IDs, not reusable OS identifiers. `Sleep(int)` requires a non-negative duration
and parks during the OS wait; `Yield()` polls then yields. Default thread handles,
null actions, self-join, invalid mutex ownership, and uninitialized atomic cells
raise `Exception`. The parameterless core Exception constructor has a distinct
native overload, including for implicit derived-exception constructor chaining.

`Mutex.Lock()`/`Unlock()` provide recursive exclusion and release/acquire
publication. Waiters park; a collector can stop the owner while it holds the
application mutex. Pair acquisition/release using `finally`. Exiting or detaching
while owning a mutex fails with a diagnostic; mutex state owns no persistent OS
handle and needs no disposal. `AtomicInt` offers `Value`, `Increment`, `Decrement`,
`Exchange`, and `CompareExchange(value, comparand)`. Operations are serialized
under the runtime mutex with sequentially consistent publication. Adjustments
wrap in the 32-bit representation. Collections and other mutable application
objects require their own synchronization.

The first managed entry implicitly attaches the startup thread for compatibility.
Other OS threads must call `cx_gc_attach_thread` before any managed entry and
`cx_gc_detach_thread` before exiting. Attach is idempotent while attached; detach
requires an attached, running thread. Attachment records native stack bounds.
Registration alone does not root pointers in foreign global storage. Retained
pointers must stay in a registered static root or a reachable managed block.
Detached stacks are no longer roots. Publish any surviving references before
detaching. OS thread identifiers are not used as ownership tokens, so identifier
reuse cannot authorize a new, unregistered thread.

Keep incoming worker/callback arguments rooted in shared storage or on the
publishing thread until attachment completes: an unattached thread waiting for
an ongoing collection is not yet part of the root set.

For foreign callbacks, `cx_gc_enter_thread()` returns whether it attached the
OS thread; pass that result to `cx_gc_leave_thread()` so nested callbacks on a
running CX thread do not detach their caller. Establish an exception boundary,
contain failures before returning through a foreign ABI, and leave scopes in
reverse order. Detach rejects active exception frames and clears pending TLS
state. A callback from inside the public native-only blocking wrapper remains
invalid. Reentrant native calls use the separate `cx_gc_run_native_call` adapter;
guarded callbacks resume managed execution before entering CX. See
[native-interop-design.md](native-interop-design.md) for the current interop contract.

`cx_gc_add_root(address, size)` creates a removable native root handle for stable,
aligned storage; `cx_gc_remove_root(handle)` ends its lifetime. Retained function
values require rooting the entire value, including environment, target and
invocation list. The storage must outlive the registration and must be mutated
only by a running registered thread with appropriate synchronization. Remove a
stack-region handle before the region expires; retain native callback contexts
until the native library confirms no callback can use them. Persistent module
roots continue to use `cx_gc_register_static_roots` and prevent module unloading.

## Cooperative stop-the-world collection

Registered threads are running or parked. A collector requests a stop while
holding the runtime mutex, waits until every other registered thread is parked,
then scans their saved register contexts and active stack ranges, static roots,
and managed blocks. It resumes threads after sweeping. Attach waits outside the
registry during an ongoing collection; detach participates in pending stops.
Every heap operation is a polling point. `cx_gc_safepoint` provides an explicit
poll for allocation-free native loops. A parked stack snapshot remains inside
an active runtime frame until the thread resumes; it never points into a frame
that has returned. Failure to establish stack bounds fails attachment rather
than allowing collection with an incomplete root set.

`cx_gc_run_blocking(callback, context)` saves registers and parks the caller
while invoking a native-only callback. The callback may wait on native events
or join a native worker, but must not enter CX, access managed storage, allocate,
detach, or unwind out of the wrapper. It must return normally. Context and live
managed references in the caller's stack remain roots.
Native callbacks and other threads must not modify the parked caller's stack
regions or root slots; put writable native wait state in unmanaged storage.
The wrapper waits for any collection to finish before returning to managed
execution. This persistent
frame avoids the unsafe pattern of an enter-blocking function returning with a
snapshot of its dead stack frame. Reentrant managed callbacks require returning
to running state through the separate `cx_gc_run_native_call` protocol; they are
not permitted inside `cx_gc_run_blocking`.

Cooperation can wait indefinitely if a running thread neither polls nor uses the
blocking wrapper. The runtime never forcibly suspends a thread holding an OS
lock. Native embedders must satisfy this contract. Generated function entry and
supported loop bodies poll through the runtime. Static and DLL module
initialization and function-value reflection descriptors use the same GC-aware
once gate, including same-thread recursion and cached initializer failures.
Waiters park while another thread initializes. The generic virtual registry is
serialized, pending exception objects are explicit per-thread TLS roots, and
Console I/O parks through native-only callbacks with unmanaged write buffers.

## Task and foreign-thread boundaries

The task executor claims one owner OS thread. Timer, external-event,
cancellation, completion, and continuation callbacks run on that thread.
Event providers wait in a GC blocking region and report readiness without
running managed code. The scheduler does not provide worker-pool execution or
continuation migration.

Thread-local exception frames remain on their OS thread; an exception cannot
unwind another thread's stack. Cancellation is cooperative, rather than
asynchronous termination.

`cxnet` owns socket polling and registers opaque wait contexts with the generic
executor boundary. Pending operations retain their buffers and completion
state until readiness is removed and the task completes. OS readiness does not
directly complete a managed task from a foreign thread.

Generated synchronous entry points drain or cancel executor-owned work during
shutdown. An unjoined task is otherwise detached from its caller; callers that
need a completion boundary must join or await the operation explicitly.

Foreign callbacks attach before entry, obey polling rules, and contain
exceptions before returning through the foreign ABI. Attachment scopes,
removable native roots, and the guarded native-call adapter provide the
lifecycle operations described in [native interoperability](native-interop-design.md).

## Platform coordination and limits

Windows uses SRW locks and condition variables; Linux uses pthread mutexes and
condition variables. Wait predicates are rechecked after waking, including
spurious wakeups.

Collection requires explicit native cooperation. A thread that neither polls
nor enters a supported blocking region can delay collection indefinitely.
Conservative scanning may retain unreachable objects, application data still
needs synchronization, and modules with registered roots cannot be unloaded.
