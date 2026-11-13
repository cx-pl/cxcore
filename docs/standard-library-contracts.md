# Standard-library runtime contracts

These contracts describe the current C runtime. CX allocations are managed by a
non-moving conservative collector with explicit native worker registration and
cooperative stop-the-world coordination and public `System.Threading` workers,
mutexes, and atomic integer cells. The collector design and limits are documented in
[`managed-memory-design.md`](managed-memory-design.md). The ABI and ownership
rules remain experimental.

## Allocation and ownership

- `Memory.Alloc` returns zero-initialized, collector-tracked storage. A zero-byte request returns a
  non-null allocation that can be passed to `Memory.Free`.
- `Memory.Realloc(block, 0)` frees `block` and returns null. For a nonzero size, a
  failed reallocation terminates the process; it never returns a lost old pointer.
- `Memory.Copy` is `unsafe` and is a no-op for size zero. Nonzero copies require
  valid, non-overlapping ranges, as with C `memcpy`; the runtime also scans copied
  pointer-sized windows and rejects managed-reference escapes into durable storage.
- `Memory.Free` and `Memory.Realloc` are `unsafe`: freeing live managed storage or
  retaining aliases invalidated by a moving resize is invalid. Arena allocations
  cannot be individually freed; `Realloc` follows the owning arena when it grows.
- `NativeBuffer.FromUtf8` and `FromUtf16` copy their input into owned native
  storage and carry `@NoEscape` contracts. The returned buffer owns that native
  storage and must be disposed before its managed owner arena is released.
- Built-in `T[]` storage and the public `System.Collections.Array<T>` API use
  the same `System.Collections.Array` runtime structure and own a
  zero-initialized element buffer allocated by `cx_array_new`. The public type
  is declared in `src/System/Collections/Array.cx` and exposes `Length`, `Get`,
  and `Set`. The current explicit cleanup
  is `Memory.Free(array->_data)` followed by `Memory.Free(array)`.
  `cx_array_at` returns null for a null array, an out-of-range index, invalid
  zero-sized elements, or missing backing storage.
- Runtime-owned allocations go through `Memory.Alloc` and are eligible for collection
  when unreachable. `Memory.Free` immediately unregisters and releases a block; freeing
  a block while a live reference may be used is invalid. `Memory.Realloc` tracks the
  resized block, but references invalidated by a move must not be used.
- `Memory.WithArena(action)` runs a `delegate() => void` synchronously with managed
  allocations routed to a growable temporary arena. Arena references may be used
  during the callback; storing them in durable objects or caller-frame stack slots
  is rejected by runtime checks. The callback cannot return a value. On normal
  return or a CX exception, the allocation context is restored and arena storage is
  released; exceptions are rethrown after cleanup. Use `try`/`finally` or
  `IDisposable` inside the callback to release native resources before arena data.
- `Memory.WithArena(capacity, action)` runs the same callback in fixed-capacity,
  preallocated storage. Capacity exhaustion throws a CX exception and does not grow
  the arena. Per-allocation ownership extents and alignment padding consume this
  capacity; the arena and its single chunk descriptor are fixed setup overhead
  allocated before the callback, with no per-allocation native metadata allocation.
- `MemoryArena(initialCapacity)` is the reusable object-oriented form. Its
  constructor reserves the initial chunk; call `Run(action)` for each temporary
  allocation generation and `Dispose()` when finished. `Run` clears allocation
  extents on normal and exceptional exit while retaining chunks for the next run.
  References created in one run expire when that run returns. The object is
  non-reentrant, must not be disposed while running, and must be explicitly
  disposed because it has no finalizer. A bounded overload accepts
  `MemoryArena(initialCapacity, maximumCapacity)`.

  ```cx
  MemoryArena arena = new MemoryArena(4096u);
  try {
      arena.Run(() => { /* temporary work */ });
      arena.Run(() => { /* reuses the retained chunk */ });
  } finally {
      arena.Dispose();
  }
  ```
- `Memory.WithDurable(action)` temporarily routes allocations and owner-following
  buffers to the ordinary GC heap, even when called inside an arena. This supports
  constructing a new durable graph synchronously. Runtime barriers still reject
  storing an arena reference into that durable graph. The callback cannot return a
  value; copy or construct the values needed by the durable graph inside it.
- `Nullable` is a one-pointer value. Its constructor and `Value` setter store a
  non-owning pointer; compiler-generated lifts call `Nullable<T>.CreateValueStorage`,
  which allocates a copy of the supplied bytes through `Memory.Alloc`; the payload is
  traced conservatively with the rest of the managed heap.
- `Object` initializes its runtime vtable. Ordinary objects leave `_box` null;
  boxed value objects use it to retain their managed box header and payload.
  No finalizers run. The non-moving collector may retain unreachable blocks when stale
  pointer-shaped values remain on a scanned stack or in a registered root.
- `DateTime` component construction validates the Gregorian date and time fields. `AddMonths`
  uses calendar months, clamps the day to the last day of the destination month, and keeps
  the time-of-day and offset. Invalid components and results outside the stored millisecond
  range write a diagnostic and exit with failure.

## Strings, characters, and console

- String data is UTF-8 and `_length` is a byte count. `String.Length` counts Unicode
  scalar values, and `String.Item` indexes those scalar values. The item getter returns
  `-1` for an invalid index or malformed UTF-8 sequence. `Length` counts each malformed
  byte as one replacement position so it stays bounded by the stored byte length.
- `String ==`, `String !=`, `Equals`, and `GetHashCode` use the stored UTF-8 bytes;
  equal byte sequences compare equal and produce the same hash. `ToString()` returns
  the same string instance. String constructors initialize the `String` vtable after
  the base `Object` constructor initializes the base object state.
- `String <`, `<=`, `>`, and `>=` compare UTF-8 byte sequences lexicographically.
  For valid UTF-8, this gives Unicode scalar value ordering. A null string sorts
  before a non-null string when the comparison is made through typed string values.
- `String + String` copies both UTF-8 byte sequences into a new managed string.
  A null operand contributes no bytes. Length overflow and allocation failure
  terminate according to the runtime allocation contract.
- `String.__constructor(length, data)` borrows an immutable byte buffer. The buffer
  must remain alive and unchanged while the string uses it. String literals borrow
  static storage. The character-fill constructor allocates its encoded UTF-8 buffer;
  callers may release it with `Memory.Free` only when no live reference can use it.
- `Char` is a Unicode scalar value in a 32-bit integer. `Char.NumBytes` returns its
  UTF-8 encoded width, or `-1` for a surrogate or value above U+10FFFF. Current
  `IsDigit`, `IsLetter`, case conversion, and whitespace helpers are ASCII-only.
- `Console.Read(length)` reads at most `length` bytes. `ReadLine()` reads through a
  newline or EOF and grows its buffer as needed. Both return a heap-allocated String
  object with heap-allocated contents; those allocations are traced. Callers may
  release them with `Memory.Free` when no live reference can use them. `Console.Write`
  writes the stored UTF-8 bytes
  unchanged and returns the number of bytes written.

Allocation failures in `Memory.Alloc` and nonzero `Memory.Realloc` write `Out of memory`
to standard error and exit with failure. The old allocation remains untouched when
reallocation fails before process termination. Bounds,
ownership, and UTF-8 errors do not currently raise typed CX exceptions.

## Math

All methods declared in `src/System/Math.cx` have native implementations in
`platform/common/System/Math.c`, along with `E`, `Pi`, `Tau`, `Sqrt2`, and `Sqrt3`.
Integer `Floor` and `Ceiling` return the input unchanged. Integer `Abs` rejects
the minimum signed value, whose magnitude cannot be represented. `Clamp` rejects
`min > max`; floating comparisons leave a NaN input unchanged and ignore a NaN
bound. Floating `Min` and `Max` propagate NaN, with negative zero sorting below
positive zero.

`Round` uses nearest, ties-to-even rounding and preserves negative zero. The
decimal-place overload accepts 0..6 places for float and 0..15 for double;
values with magnitude at least 1e8 (float) or 1e16 (double) are returned unchanged.
These rounding limits and special-value rules follow the
[.NET 10 Math](https://github.com/dotnet/runtime/blob/v10.0.0/src/libraries/System.Private.CoreLib/src/System/Math.cs)
and [MathF](https://github.com/dotnet/runtime/blob/v10.0.0/src/libraries/System.Private.CoreLib/src/System/MathF.cs)
contracts. Invalid integer `Abs`, clamp bounds, and rounding precision throw a
managed `System.Exception` with a diagnostic message.

`Log(value)` is the natural logarithm. The base overload uses the quotient of
natural logarithms, returning NaN for base 1, negative bases, and base zero or
positive infinity when the value is not 1. Negative logarithm inputs produce
NaN and zero produces negative infinity. Reciprocal estimates use the portable
fallbacks `1 / value` and `1 / sqrt(value)`. Floating domain errors use IEEE
NaN/infinity results rather than managed exceptions.

## Reflection values

`TypeInfo.ConvertValue` is annotated `@NoEscape`: it reads the supplied object
and does not retain it. Compatible reference conversions may return the same
object, so the result keeps the input's lifetime; numeric conversions allocate
a boxed value in the active allocation context. The annotation permits a
scoped input to be converted, but does not make the returned object durable.

## UUID generation

`System.Uuid` stores four unsigned 32-bit words in canonical UUID order. Its
`ToString()` returns lowercase `8-4-4-4-12` text; `Version` reads the version
field. `Zero` remains available, and the standard `NamespaceDns`, `NamespaceUrl`,
`NamespaceOid`, and `NamespaceX500` constants support name-based UUIDs.

| Method | Inputs and behavior |
| --- | --- |
| `NewUuidV1()` | Gregorian 100 ns time, synchronized state, random multicast node |
| `NewUuidV2(byte localDomain, uint localIdentifier)` | DCE security layout; local identifier replaces the low timestamp and domain replaces the low sequence byte; fresh random multicast node |
| `NewUuidV3(Uuid namespaceId, string name)` | MD5 of namespace wire bytes followed by the complete UTF-8 name bytes |
| `NewUuidV4()` | OS cryptographic random source |
| `NewUuidV5(Uuid namespaceId, string name)` | SHA-1 of namespace wire bytes followed by the complete UTF-8 name bytes |
| `NewUuidV6()` | Reordered Gregorian timestamp with synchronized state and random multicast node |
| `NewUuidV7()` | Unix milliseconds with a random 74-bit suffix that increases within the same millisecond or during clock rollback |
| `NewUuidV8()` | Custom layout populated from the OS random source |
| `NewUuidV8(ulong customA, ushort customB, ulong customC)` | Caller-supplied 48-, 12-, and 62-bit fields; out-of-range fields are rejected |

All generators set the RFC variant and their version bits. Versions 1, 6, and 7
serialize access to generator state; versions 3 and 5 are deterministic. UUID
names are used exactly as supplied, with no case conversion or Unicode
normalization. Embedded NUL bytes are included. Invalid arguments or OS source
failures raise `System.Exception`. Generator state is process-local and is not
persisted across restarts. Version 2 domain values 0, 1, and 2 identify person,
group, and organization; other byte values are application-defined.

Layouts and name-based test vectors follow
[RFC 9562](https://www.rfc-editor.org/rfc/rfc9562.html); version 2 uses the DCE
security layout referenced by that specification.

## Numeric TryParse

`Byte`, `SByte`, `Short`, `UShort`, `Int`, `UInt`, `Long`, `ULong`, `Float`,
`Double`, and `Decimal` expose `public static extern T? TryParse(string text)`.
The result is null for missing input, malformed text, or overflow, and otherwise
contains a managed copy of the parsed value. Parsing failures do not throw or
change the current exception. This API uses a nullable return because CX does
not have `out` parameters.

Parsing uses invariant ASCII syntax and permits surrounding ASCII whitespace
and one leading sign. Integers require base-ten digits. Float, double, and
decimal additionally accept a decimal point and an optional signed exponent.
The entire input must match: embedded NULs, grouping separators, hexadecimal
text, suffixes, `NaN`, and infinity are rejected. Floating underflow can produce
zero, while floating overflow is rejected. Decimal parsing retains decimal
precision and applies nearest rounding with ties to even when needed. Parsing
and formatting helpers do not change the process locale.

```cx
import System;

int? parsed = Int.TryParse("42");
int value = parsed ?? 0;
```

## StringBuilder

`System.StringBuilder` is a managed class with an empty constructor and a
`constructor(uint capacity)` overload. It grows its UTF-8 buffer geometrically,
so repeated small appends take amortized linear time in the final output size.
`Length` counts Unicode scalar positions, following `String.Length` (malformed
UTF-8 bytes count as individual positions). `Capacity` and `EnsureCapacity`
measure allocated bytes, and may return more capacity than requested.

`Append` accepts strings, characters, booleans, all eight signed/unsigned integer
types, float, double, and decimal.
Numeric formatting is invariant; floating values use round-trip precision.
`AppendLine()` and `AppendLine(string)` append the platform newline. A null
string contributes no bytes. `Append`, `AppendLine`, and `Clear` return the
builder for chaining. `Clear` keeps the allocated capacity for reuse.

`ToString()` copies the current bytes into an immutable managed snapshot, so
later appends, clears, or reallocations cannot change an earlier result. Empty
snapshots use `String.Empty`. A builder supports one mutating caller at a time;
different builder instances can be used independently by different threads.
The sealed builder's `ToString()` override is annotated `@NoEscape`: it reads
the builder synchronously and copies its bytes, so it does not retain the
builder or a borrowed receiver.

```cx
StringBuilder builder = new StringBuilder();
builder.Append("Total: ").Append(123.45m).AppendLine();
string result = builder.ToString();
```

## Shared-library declarations

`CX_API` marks runtime functions as exports while building `cxcore`, imports for dynamic
consumers, and undecorated declarations for static consumers. Public runtime data uses
`CX_CXCORE_DATA_API` with the same producer/consumer/static behavior. CMake propagates
`CX_STATIC_LINK` from static `cxcore` targets to their consumers.

## Managed-memory limits

The collector scans registered thread stacks, saved registers, managed blocks, and
registered static-field regions. Generated modules register their static fields;
native C code must register static storage that retains managed pointers. Heap
operations from an unregistered OS thread fail. Native workers must attach,
poll safepoints, park during native-only blocking calls, and detach before exit;
see [threading-gc-design.md](threading-gc-design.md). Application fields require
their own synchronization. Synchronous native calls on a running attached thread
retain their stack roots; references retained beyond the call must be kept in a
registered root or another managed block. Foreign callbacks use owned attachment
scopes and removable root handles. Reentrant native imports use
`cx_gc_run_native_call`; guarded callbacks resume managed execution before entering
CX. The native-only `cx_gc_run_blocking` callback must not enter CX. See
[native-interop-design.md](native-interop-design.md) for the callback contract.

## Platform support

The runtime supports Windows through MSVC and x64 Linux through GCC. Linux
selects native environment and system-clock providers and the platform-specific
managed-memory helpers. Static and shared Linux targets build and pass the native
runtime test suite in the repository's Docker development container. macOS
remains unsupported.

## Native interop and text

wchar is a 16-bit UTF-16 code unit on every target. System.WideString owns immutable
copied storage; Length/Item count units and ScalarCount/ScalarAt validate scalars.
Equality, hashing and ordinal comparison use units. Empty values and embedded zeros
are preserved. String.ToWideString and the Object.ToString override on WideString
transcode explicitly, without normalization or implicit conversion. Malformed UTF-8
or unpaired UTF-16 throws EncodingException with a byte/unit Position.

System.Interop.NativeBuffer owns malloc storage. AllocateAligned takes a power-of-two
alignment for stronger SDK layouts. FromUtf8/FromUtf16 copy input with optional
termination; ByteLength includes that terminator. ToUtf8String/ToWideString copy
explicit output lengths without scanning for zero. Dispose is explicit/idempotent;
never dispose while native code retains Data. SDK objects and buffers require their
own allocator/destructor, not Memory.Alloc or NativeBuffer.Dispose.

NativeLibrary keeps loaded handles until process exit; Resolve<T> returns typed
native functions or throws a load/symbol error. NativeError holds immediate TLS
errno/last-error snapshots. NativeCallback.Register<T> roots a closure and optional
owner until successful SDK unregistration and completion of in-flight callbacks.
Unregister failure permits retry; self-unregistration is rejected. LastException
survives close, and late callbacks receive their configured failure result. Small
context tombstones stay until process exit; managed payloads are freed on close.
Callbacks catch CX exceptions before foreign frames and restore native TLS state.
See [native-interop-design.md](native-interop-design.md) for ABI and lifetime rules.
