# Standard exception hierarchy design

This document defines the shared exception contract used by the runtime and
standard library, including filesystem APIs. It deliberately covers common
library and runtime failures rather than trying to reproduce every Java or
.NET exception.

## Public hierarchy and ownership

All types are classes under `System` unless a namespace is shown. `Exception`
remains the root, so existing user subclasses and `catch (Exception)` continue
to work. Constructors are declared independently on each public type; they are
not inherited.

| Type | Base | Context |
| --- | --- | --- |
| `System.Exception` | `Object` | `Message`, `InnerException`, `RuntimeType`, `StackTrace` |
| `System.ArgumentException` | `Exception` | `ParamName` |
| `System.ArgumentNullException` | `ArgumentException` | `ParamName` |
| `System.ArgumentOutOfRangeException` | `ArgumentException` | `ParamName`, optional boxed `ActualValue` |
| `System.InvalidOperationException` | `Exception` | Invalid current state |
| `System.ObjectDisposedException` | `InvalidOperationException` | `ObjectName` |
| `System.NotSupportedException` | `Exception` | Capability/platform limitation |
| `System.NotImplementedException` | `Exception` | Explicitly unfinished implementation |
| `System.FormatException` | `Exception` | Invalid input to throwing parse/conversion API |
| `System.ArithmeticException` | `Exception` | Arithmetic failure base |
| `System.DivideByZeroException` | `ArithmeticException` | Zero divisor |
| `System.OverflowException` | `ArithmeticException` | Checked arithmetic/conversion overflow |
| `System.InvalidCastException` | `Exception` | Failed checked cast at a safe managed boundary |
| `System.IndexOutOfRangeException` | `Exception` | Selected generated bounds check |
| `System.NullReferenceException` | `Exception` | Selected generated null check |
| `System.Collections.KeyNotFoundException` | `Exception` | Throwing collection key lookup |
| `System.TimeoutException` | `Exception` | Operation timeout; distinct from cancellation |
| `System.UnauthorizedAccessException` | `Exception` | OS permission denial |
| `System.IO.IOException` | `Exception` | General I/O failure; owns native error context |
| `System.IO.FileNotFoundException` | `IOException` | `FileName` |
| `System.IO.DirectoryNotFoundException` | `IOException` | Optional path |
| `System.IO.DriveNotFoundException` | `IOException` | Optional drive/path |
| `System.IO.PathTooLongException` | `IOException` | Optional path |
| `System.IO.EndOfStreamException` | `IOException` | Required data ended early |
| `System.IO.InvalidDataException` | `Exception` | Malformed external data, not transport failure or premature EOF |
| `System.EncodingException` | `Exception` | Retain `Position` and current identity |
| `System.Threading.Tasks.OperationCanceledException` | `Exception` | Retain namespace, token, and current identity |
| `System.Net.Sockets.SocketException` (`cxnet`) | `IOException` | Socket native error details; owned by `cxnet` |

`cxcore` must not reference `cxnet`. The cancellation type remains in its
existing task namespace for this delivery; its difference from .NET is
intentional and source/binary compatibility takes priority over namespace
alignment.

## Constructors and message contract

The signatures below use CX types (`string`, nullable `Exception?`, and boxed
`object?`). `message` parameters are nullable in behavior even if a particular
compiler build does not yet express nullable annotations. Every overload must
initialize base state and the most-derived runtime type. Constructors are not
inherited.

| Type(s) | Public constructors |
| --- | --- |
| `Exception` | `()`, `(string message)`, `(string message, Exception? innerException)` |
| `ArgumentException` | `()`, `(string message)`, `(string message, Exception? innerException)`, `(string message, string? paramName)`, `(string message, string? paramName, Exception? innerException)` |
| `ArgumentNullException` | `()`, `(string? paramName)`, `(string message, string? paramName)`, `(string message, string? paramName, Exception? innerException)` |
| `ArgumentOutOfRangeException` | `()`, `(string? paramName)`, `(string message, string? paramName)`, `(string message, string? paramName, object? actualValue)`, `(string message, string? paramName, object? actualValue, Exception? innerException)` |
| `InvalidOperationException`, `NotSupportedException`, `NotImplementedException`, `FormatException`, `ArithmeticException`, `DivideByZeroException`, `OverflowException`, `InvalidCastException`, `IndexOutOfRangeException`, `NullReferenceException`, `System.Collections.KeyNotFoundException`, `TimeoutException`, `UnauthorizedAccessException`, `System.IO.IOException`, `System.IO.InvalidDataException` | `()`, `(string message)`, `(string message, Exception? innerException)` |
| `ObjectDisposedException` | `()`, `(string? objectName)`, `(string message, string? objectName)`, `(string message, string? objectName, Exception? innerException)` |
| `System.IO.FileNotFoundException` | `()`, `(string message)`, `(string message, Exception? innerException)`, `(string message, string? fileName)`, `(string message, string? fileName, Exception? innerException)` |
| `System.IO.DirectoryNotFoundException`, `DriveNotFoundException`, `PathTooLongException` | `()`, `(string message)`, `(string message, Exception? innerException)`, `(string message, string? path)` |
| `System.IO.EndOfStreamException` | `()`, `(string message)`, `(string message, Exception? innerException)` |
| `System.EncodingException` | Preserve existing `(uint position)`; add `()`, `(string message)`, `(string message, Exception? innerException)`, and `(string message, uint position)` |
| `System.Threading.Tasks.OperationCanceledException` | Preserve existing `()` / token overloads; add `(string message)`, `(string message, Exception? innerException)`, `(string message, CancellationToken cancellationToken)`, and `(string message, Exception? innerException, CancellationToken cancellationToken)` |
| `System.Net.Sockets.SocketException` (`cxnet`) | `()`, `(string message)`, `(string message, Exception? innerException)`, and a native-error-context overload defined by `cxnet` |

The I/O path overloads use `path` for directory, drive, and path-length
failures; `FileNotFoundException` exposes `FileName`. Native operation/domain/
code context is populated by runtime failure factories rather than a public
constructor that pretends every platform error code has the same meaning.

An omitted message selects a stable, nonempty type-specific default. An
explicitly null message follows the same defaulting rule; callers use
`Message` for display and typed catches/fields for control flow. The runtime
does not promise localized messages. `InnerException` may be null. A missing
context value is represented as null and is not reconstructed by parsing text.
`ArgumentOutOfRangeException.ActualValue` is nullable boxed `object`; it is
retained only when supplied.

## Catch and rethrow example

The concrete catch precedes broader handlers. A bare `throw;` preserves the
original exception object and its recorded origin; wrapping is explicit when a
caller needs to add context.

```cx
try {
    ReadRecord(index);
} catch (ArgumentOutOfRangeException error) {
    string parameter = error.ParamName;
    throw;
} catch (IOException error) {
    throw new InvalidOperationException("Could not read the record.", error);
} catch (Exception error) {
    LogUnexpected(error);
    throw;
}
```

For API validation, construct the argument subtype with its parameter name:

```cx
if (buffer == null) throw new ArgumentNullException("buffer");
if (offset > buffer.Length) {
    throw new ArgumentOutOfRangeException("offset");
}
```

Native failure context belongs to `IOException` and `SocketException` as
appropriate: operation name, native error domain, native numeric code, and
optional path. The domain is an enum/value identifying the source API (for
example POSIX `errno`, Win32 `GetLastError`, or a socket backend), not a
platform-independent HRESULT. Capture the native code before cleanup or any
other native call can overwrite it. Filesystem-specific mapping and path policy
belong with the filesystem implementation.

## Runtime behavior

Exception inheritance matching follows the full runtime base chain across
modules. Exact, ancestor, and unrelated matches, source-order handler choice,
filters, nested handlers, bare rethrow, and `finally` behavior retain the
existing runtime contract. Throwing preserves object identity and origin;
task propagation does not aggregate or wrap failures. Uncaught diagnostics
should show qualified type, message, inner cause, and available source
location. `StackTrace` describes only locations recorded by the runtime; it is
not a promise of platform stack unwinding.

Typed exceptions cover only checked, recoverable conditions. Runtime handler
corruption, collector corruption, unsafe native faults, and allocation failure
remain fatal unless separately designed. A managed `throw null` is a fatal
programming error: the runtime prints `Cannot throw a null exception` and
aborts before consulting the handler chain. It is not reported as an uncaught
managed exception. This is tested separately from fatal handler corruption.

## Throw-site migration inventory

This is the initial source inventory. Each entry is migrated by failure
condition, not by mechanically replacing every base exception.

| Area / current location | Current behavior | Target and contract |
| --- | --- | --- |
| Argument checks throughout `src/System/Collections/Query/EnumerableQueryExtensions.cx` and `AsyncEnumerableQueryExtensions.cx` | Already throw `ArgumentNullException` / `ArgumentOutOfRangeException`; nonthrowing query probes remain unchanged | Keep typed failures and verify contextual `ParamName` / range values |
| `src/System/CloneContext.cx` | Generic `Exception` for invalid limits, null values, duplicate/bad entries, cycles, and unbalanced depth | Argument exceptions for caller inputs; `InvalidOperationException` for graph/state failures |
| `src/System/DependencyInjection/ServiceCollection.cx`, `ServiceDescriptor.cx` | Generic `Exception` for null descriptors/factories, invalid type/lifetime/activation, reserved keys, and incompatible registrations | Argument exceptions for invalid supplied values; `InvalidOperationException` for invalid registration state; preserve documented resolution wrappers |
| `src/System/Threading/Tasks/Task.cx` | Generic exceptions for invalid `WhenAll` arrays/entries and shared `ThrowIfAlreadyCompleted` / `ThrowIfNotCompleted` checks | Argument exceptions for inputs; `InvalidOperationException` for stale, repeated, or incomplete state; retain cancellation/fault distinction |
| `platform/common/System/Math.c` | Base `Exception` for signed `Abs` overflow, invalid `Clamp` bounds, and invalid rounding precision | `OverflowException` for signed minimum absolute value; `ArgumentOutOfRangeException` for invalid bounds/precision |
| `platform/common/System/Reflection/TypeInfo.c`, `ModuleInfo.c` | Generic reflection failures for incompatible values, arguments, array readers, metadata, and indices | `InvalidCastException` for conversion; `ArgumentException` for invocation/array inputs; `InvalidOperationException` for unavailable metadata/readers; `ArgumentOutOfRangeException` for indices |
| `platform/common/System/Interop/NativeLibrary.c` | Generic failures for invalid names and load/resolve errors | `ArgumentNullException` for missing names; `ArgumentException` for malformed names; `InvalidOperationException` when the OS cannot load/resolve the requested native item |
| `platform/common/System/Uuid.c` | One generic helper merged invalid name arguments and platform generation failures | `ArgumentNullException`/`ArgumentException` for name inputs; `InvalidOperationException` for OS random/time failures |
| `platform/common/cxinterop.c` | Generic failures for invalid native function pointers and invalid stack allocation arguments; allocation failure was also a recoverable generic exception | `ArgumentException` for invalid function pointers, `ArgumentOutOfRangeException` for stack sizing, and fatal handling for native context allocation failure |
| `platform/common/System/Diagnostics/MonotonicClock.c` | Base exception when the OS monotonic clock call fails | `InvalidOperationException` for an unavailable runtime clock |
| `platform/common/System/Decimal.c` | One helper allocates base `Exception` for malformed decimal values, arithmetic overflow, and division by zero | Split into `FormatException`, `OverflowException`, and `DivideByZeroException`; `TryParse` stays nonthrowing |
| `platform/common/System/Memory.c` and `cxarena.c` | Arena APIs previously used message-only base helpers for null actions, invalid capacities, disposed/running arenas, callback capture, and capacity exhaustion | `Memory.c` now raises argument, disposed-object, null-reference, or invalid-operation types by condition. GC corruption, allocation failure, and arena capacity exhaustion remain fatal invariants |
| `platform/common/System/Threading/Threading.internal.h` callers and task validation in `platform/common/System/Threading/Tasks/Task.c` | Shared message helper and state checks currently reach base exception behavior | Use `InvalidOperationException` for scheduler/task state; cancellation continues to use the existing cancellation type |
| `src/System/EncodingException.cx`, `platform/common/System/EncodingException.c` | Typed exception with `Position`; native malformed-input entry points allocate this type | Keep type and position; add standard constructor/message behavior without changing identity |
| `platform/common/cxinterop.c` and general `cx_exception_raise_message` | Generic failures at interop/callback boundaries | Classify only where the boundary has a recoverable, documented condition; preserve callback barriers and cleanup-before-throw |
| Compiler checked-cast and generated bounds/null checks (`cxc/src/CxCompiler`) | Some checks terminate or use generic runtime failures; coverage is uneven | Convert only the selected safe managed checks to `InvalidCastException`, `IndexOutOfRangeException`, or `NullReferenceException`; preserve single evaluation and document unchecked operations |
| `cxnet/src/System/Net/Sockets/Socket.cx` and `cxnet/platform/*/SocketPlatform.c` | Generic exceptions and discarded native socket error codes | Shared argument/disposed/state exceptions; native operations retain POSIX `errno` or Winsock code/domain and CX completions create `cxnet`-owned `SocketException(IOException)` |
| OS filesystem failures | No `cxcore` filesystem operations are present | Map OS failures to this hierarchy in the filesystem implementation; do not add filesystem throw sites here |

The migration targets above are implemented for this delivery; the first two
columns preserve the audit baseline and are not a list of outstanding generic
failure paths. `CloneContext`, dependency-injection validation, task completion
state, math/decimal, reflection, native-library loading, UUID generation,
interop validation, monotonic-clock errors, memory/arena checks, threading,
encoding, selected generated checks, and `cxnet` now use their documented
types. Native text-buffer checks use typed argument, range, index, overflow,
format, disposed-object, null-reference, or I/O failures. Native allocation
failure and arena-capacity exhaustion remain fatal. `cx_exception_raise_message`
is retained for explicitly generic public/runtime boundaries; remaining uses
are not a substitute for an inventoried recoverable library check.

## Source migration notes

Existing `catch (Exception)` code continues to catch the new subclasses. Code
that depended on a failed checked cast terminating the process now receives
`InvalidCastException` and runs enclosing managed cleanup; null checked casts
and nonthrowing safe casts keep their prior results. Invalid API inputs and
invalid object state now expose argument or state subtypes, so callers should
catch by type and use `ParamName` or other context properties instead of
matching message text. `Try...` APIs remain nonthrowing. The cancellation type
keeps its existing namespace and identity. The ABI number remains 1 for this
unpublished checkout, but binaries must be rebuilt together while the
exception object layout is in development.

Nonthrowing contracts to preserve include `TryParse`, `TryAdd`, safe casts,
optional/default lookups, ordinary EOF, and `Exists`-style probes. Preserve
floating-point IEEE behavior and unchecked arithmetic. Add overflow guards
before selected integer operations that could invoke C undefined behavior;
never attempt to recover after an unsafe native instruction.

## Compiler, metadata, and native integration

Public declarations live below `src/System/...`; native implementations mirror
them below `platform/common/System/...` with guarded Windows/Linux files when
needed. Keep source registration unconditional under the existing core CMake
and source-layout rules. Update generated declarations, headers, vtables,
reflection metadata, type sizes, base links, constructor invokers, and exported
symbols as one change. The synthetic declarations in
`cxc/src/CxCompiler/Semantics/Binding/SemanticBinder.Text.cs` must agree with
the public API; prefer deriving them from a shared declaration source or add a
parity gate rather than allowing a second hierarchy to drift.

Native helpers must allocate the correct derived size, initialize constructors
and vtables, root objects/context through GC, and throw on the attached CX
thread. Release temporary storage, native handles, and locks before a managed
throw can `longjmp` past cleanup. Blocking workers and foreign callbacks return
failure state through the established attachment/barrier instead of unwinding
through unsupported frames.

## Implementation requirements

1. This design and the inventory above establish the supported initial scope.
   Keep remaining runtime-check gaps explicit.
2. Implement the public shared and I/O hierarchy, constructors, metadata,
   contextual fields, uncaught formatting, and compiler declaration parity.
   Construct and catch `FileNotFoundException` before filesystem delivery.
3. Migrate decimal/math, selected collection/iterator/task/scheduler checks,
   encoding diagnostics, arena/disposal checks, and `cxnet` socket failures.
4. Convert selected checked-cast and generated bounds/null failures at safe
   managed boundaries. Keep fatal invariants fatal.
5. Publish examples and migration notes, and pass the native, compiler, and
   platform validation gates before considering the implementation complete.

## Current implementation status

The shared hierarchy, contextual properties, the in-development ABI 1 exception
layout, compiler fallback declarations, uncaught diagnostics, selected
decimal/math/task/scheduler/reflection/interop/cast and arena migrations, and
typed `cxnet` validation/native failures are present. ABI remains version 1;
the checkout is unpublished and the exception layout is still in development.
The compiler suite passes 1,537/1,537. Core CTest passes 164/164 on Windows
x86/x64 static/shared and 165/165 on Linux x64 static/shared. Socket CTest
passes 3/3 on Windows x86/x64 and Linux x64, each under static and shared
linking. The expanded cross-project fixture passes Windows x86/x64 and Linux
x64, each under static and shared linking.

The strict generated-C gate passes for `cxcore`, `cxnet`, and the
`streamingRead` consumer on Windows x86/x64 and Linux x64, each with static and
shared declarations. The scripts compile the generated project translation
unit (`cxcore.c`) so project-level string definitions and generated functions
are checked together.
Generated iterator `IEnumerator<T>.Current` now throws
`System.InvalidOperationException` outside a successful move; a focused
compiler regression checks the lowered code. This replaces the earlier generic
`System.Exception` behavior. The legacy coroutine native runner
currently stops at static linking because generic symbols are duplicated
between `bench.c` and `cxcore.lib`.

Native socket operations preserve the backend error domain and numeric code;
the CX completion path constructs `SocketException`, while the native C test
harness supplies a base exception to its test-only task callback. Fatal GC
corruption, native allocation failure, and arena capacity exhaustion remain
outside allocating exception recovery. The legacy generic `cx_text_error`
helper has been removed. `NativeBuffer` validation now throws argument,
overflow, disposed-object, or I/O exceptions by condition; stream failures
retain operation, native error domain, and numeric code. `StringBuilder`
throws null-reference, argument, overflow, and format exceptions at its
checked boundaries. `WideString` reports argument, index, and overflow
failures with typed exceptions, and UTF-8 output-size overflow is typed.
Managed array receiver/index access and checked casts use typed exceptions; the
legacy raw `cx_array_at` helper remains nonthrowing, and raw pointer/fixed
array operations plus arbitrary nullable instance receivers remain unchecked.
The checked-in `cxnet` CX project compiles.

The acceptance gates in `CX_ROADMAP.md` remain authoritative, including
cross-project subclassing, forced-GC retention, exactly-once cleanup, Windows
x86/x64 and Linux x64, and static/shared linking. This document records the
initial migration inventory; new throw sites discovered during implementation
must be added with their reason and disposition.

## References

- [.NET standard exception guidance](https://learn.microsoft.com/en-us/dotnet/standard/design-guidelines/using-standard-exception-types)
- [.NET exception handling guidance](https://learn.microsoft.com/en-us/dotnet/standard/exceptions/best-practices-for-exceptions)
- [FileNotFoundException](https://learn.microsoft.com/en-us/dotnet/api/system.io.filenotfoundexception?view=net-10.0)
- [ObjectDisposedException](https://learn.microsoft.com/en-us/dotnet/api/system.objectdisposedexception?view=net-10.0)
