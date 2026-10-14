# Threading and GC coordination

Status: Milestone 16 implemented and verified on 2026-10-13. Native coordination,
compiler polling/initialization, public CX threading, and native rooting/attachment
contracts pass the completion audit below. Milestones 12 and 15 remain paused.

## Memory and execution contract

Keep the non-moving conservative collector. A process has one heap shared by
all modules and registered OS threads. Protect allocation records, static root
registration, collection thresholds, and the thread registry with one runtime
mutex. This mutex does not synchronize application fields: concurrent access to
the same mutable data requires application synchronization. No atomicity is
promised for reference, interface, or aggregate writes. Thread start, join,
mutex release/acquire, and future atomic operations will supply publication
ordering; `volatile` will not be a CX synchronization primitive.

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
invalid; Milestone 17 must provide an explicit reentry adapter for that mode.

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
to running state and are deferred to the interoperability milestone.

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

Milestone 18's initial executor can stay on one thread. A future worker executor
must attach each worker, park idle workers, root task/continuation objects in
shared managed storage, and synchronize queue publication. Thread-local
exception frames stay on their OS thread; an exception cannot unwind another
thread's stack. Cancellation is cooperative, never asynchronous termination.
Continuation migration needs heap-owned captured state, not native stack roots.
Foreign callbacks must attach before entry, obey the same polling rules, and
contain exceptions before returning through a foreign ABI. Attachment scopes
and removable native roots provide the low-level lifecycle operations; binding
annotations/adapters are Milestone 17 work and the task executor is Milestone 18.

## Delivery sequence

1. Native registration, cooperative collection, blocking waits, and native tests: implemented.
2. Compiler polling, thread-safe module/runtime initialization, native blocking
   calls and TLS exception roots: implemented.
3. CX thread start/join, synchronization, delegate lifetime and exception
   containment: implemented and verified by generated-C conformance.
4. Foreign callback scopes/root handles and task/runtime coordination contracts:
   implemented; callback binding adapters and task executors remain in 17/18.

The first slice preserves ABI 1 and existing object/array/interface layouts.
Windows uses SRW locks and condition variables; Linux uses pthread mutexes and
condition variables. Wait predicates are rechecked after waking, including
spurious wakeups ([Windows documentation](https://learn.microsoft.com/en-us/windows/win32/sync/condition-variables)).

## Verification (2026-10-13)

- Compiler build: zero warnings/errors; all 697 xUnit tests pass.
- Windows x64 Debug: all 24 enabled native CTest cases pass. The known runtime-type
  failure dialog test remains excluded. Linux x64/GCC 11 static and shared Debug
  each pass all 26 cases, including runtime-type failure diagnostics.
- Generated CX threading conformance: all three cases pass in optimized builds
  on Windows x86 static/DLL, Windows x64 static/DLL, and Linux x64 static/shared.
  Each matrix entry runs the positive fixture plus unobserved worker failure and
  unreleased-mutex failure subprocesses. Failures require exit 1 and the specified
  diagnostic; the positive fixture requires exit 0 and foreground completion output.
- The optimized Windows x64 DLL and Linux shared positive fixture each pass 20
  consecutive runs. Earlier four-native-worker stress also passed 20 optimized runs.
- The full existing `cxc/tests/native/run-entry-point-e2e.ps1` harness passes,
  including arguments/output/exit codes, all core collection fixtures, closed
  generic initialization/layouts, virtual/interface specialization and fallback,
  delegates, and project-reference calls under static and DLL linking.
- Windows C11 checks use `/W4 /WX` with documented existing warning suppressions:
  C4130, C4090/C4100 in existing runtime code, C4102/C4189 in generated cleanup/
  closure code, and C4232 for generated vtable DLL import addresses. New Memory
  and native threading test code also passed independent GCC `-Wall -Wextra -Werror`.

## Completion audit

| Roadmap requirement | Authoritative implementation and verification |
| --- | --- |
| Multithreading support | `System/Threading/*.cx`, `cxthreading.h/.c`, built-in compiler symbols, and generated conformance in all six platform/link combinations. Start publishes rooted delegate state before native startup; join, foreground exit, mutexes, atomics and failures are exercised. |
| GC safety for concurrent programs | `Memory.c` serializes heap/root/thread bookkeeping and performs cooperative stop-the-world scans. `threading_gc_tests.c` checks four workers, concurrent collect/alloc/realloc/free, stack/interior/static roots, reclamation and detach/reattach. |
| Thread registration and safepoints | Explicit attach/detach, scoped enter/leave, generated function entry and ordinary/generic loop polls, and parked blocking waits. Compiler tests cover loop continue paths; a generated allocation-free worker polls while another thread collects. |
| Managed roots | Registered module roots, pending TLS exception roots, live-worker/failure roots, and removable native regions. `runtime_once_tests.c` retains pending errors across collection and failed initialization; `foreign_thread_tests.c` retains an object after its worker stack has exited. |
| Foreign callbacks and task coordination | Ownership, retained-region lifetime, exception containment, cancellation and migration contracts above; native scope/root operations pass normal and invalid-use subprocess tests. Binding adapters and executor implementation remain in their own Milestones 17/18. |
| Plan before implementation and preserve paused work | This design preceded implementation. No async/generator grammar or Milestone 12 performance work was introduced; Milestone 15 remains paused. Broad native regression passes after correcting value-construction initialization and fixture default locals exposed by new safepoints. |

Remaining limits are explicit native cooperation, conservative false-positive
retention, application synchronization, and unsupported module unloading. They
are the defined runtime contract rather than unfinished Milestone 16 work.

The stress fixture covers parked and explicitly polling workers, stack/static/
interior roots, parent roots during native joins, competing collections, and
unreachable worker allocation reclamation. Negative subprocess checks require
both exit code 1 and the expected registration/lifecycle diagnostic. A test-only
metadata allocation failure exercises retry storage: the first block is freed
and a fresh zeroed block must be allocated before registration and return.
