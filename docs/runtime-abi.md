# CX generated-code/runtime ABI

The generated C ABI is experimental. CX has not had a release and does not yet
promise binary compatibility between releases. The current ABI version is 1.
It identifies the C calling conventions, runtime entry points, runtime type
layouts, interface reference representation, reflection layouts, object vtable,
and value boxing helpers exposed through `cx.h` and generated project headers.

Generated C uses complete identifiers with qualified name parts joined by
underscores, for example `cxcore_System_Math_Abs`. Leading underscores within
parts are preserved: the second overload is `cxcore_System_Math_Abs__2`, and
type metadata uses names such as `cxcore_System_Math___typeinfo`. Public C
headers and native implementations use the same spelling. This changes source
notation only; exported symbol names and ABI version remain unchanged.

Runtime ABI checks are implemented in `platform/common/cxabi.c`, and type,
boxing, and reflection helpers in `platform/common/cxtype.c`. The internal GC
coordination header is `platform/common/cxgc.h` and is private to the runtime build.

Generated modules expose immutable native metadata using `struct cx_module_info`
from `cxtypes.h`. Its independent `formatVersion` is 1. Each module exports
`<TOKEN>___module_info` and `__cx_module_info_<TOKEN>()`, with the token matching
the uppercase module token used by initialization. The descriptor contains UTF-8
project metadata, sorted public class/interface, struct, enum, and delegate type pointers,
and sorted project-reference providers. Nested types are included only through
public containing types. Private API-closure types and constructed generic
specializations are excluded. Delegates have the `TYPE_DELEGATE` flag; their
enumeration does not add dynamic invocation support for delegate values.

Accessors return static descriptors without module initialization or managed
allocation. Reference providers allow shared-library imports without mutable
metadata initialization. All descriptor storage belongs to its defining module
and remains valid while that module is loaded. The CX `ModuleInfo.Current()`
intrinsic wraps the calling module's descriptor in a value containing copied
managed strings and a snapshot array of its public `TypeInfo` values. The value
retains a private native descriptor pointer for `ReferenceCount`,
`GetReference(index)`, and `VersionText`. The numeric `VersionInfo` fields contain
up to four leading dotted components; `VersionText` preserves the full project
version. Invalid reference indices throw a catchable CX exception. Adding the
private pointer changes the `ModuleInfo` value layout; rebuild
clients against the updated core header.

The descriptor includes annotation applications. An application
identifies the declaring type, reflected function index, parameter index, target,
member name, and module-qualified annotation type name. A function or parameter
index of -1 means that index does not apply; the target distinguishes type,
field, property, function, constructor, parameter, and return applications.
Arguments preserve bound constant expressions and constructor defaults as UTF-8
text. `ModuleInfo.AnnotationCount` and `GetAnnotation(index)` copy these into
managed `AnnotationInfo` snapshots, including name/expression arrays. Out-of-range
indices throw a catchable exception. Annotation constructors are never invoked
by metadata access.

Each annotation argument includes a constant-value provider. It
returns typed boxed scalar/enum values, managed strings, or null and never
constructs annotation objects. `AnnotationInfo.ArgumentValues` is a managed
object-array snapshot.
String providers use explicit byte lengths and preserve embedded nulls and UTF-8
bytes. Numeric defaults retain the constructor parameter's type and range.
Generated modules check `CX_MODULE_METADATA_FORMAT_VERSION` at compilation;
regenerate older C output when changing metadata headers. The `AnnotationInfo`
value layout now includes its argument-value array, so rebuild clients as well.

The descriptor includes per-function array readers and closed generic argument providers.
`ModuleInfo.GetGenericArguments(type)` returns argument snapshots for constructed
types used by that module, or an empty array when no entry exists.
`TypeInfo.GetInterfaces()` snapshots the runtime interface map. Shared-module
initialization includes non-generic interfaces on closed generic classes.
`CanReadFunctionArray` and `GetFunctionArrayElementType` inspect supported array
return signatures. `ReadFunctionArray(owner, index, value, maximumLength)` checks
the exact array element key and bound before making an `object[]` snapshot;
value elements are boxed with canonical descriptors, reference elements retain
their objects, and interface elements retain the original object or owning box.
Unsupported signatures, mismatched arrays, null, and excessive lengths throw
catchable exceptions. These operations do not invoke the reflected function.
The `ModuleInfo` snapshot/read methods carry the trusted `NoEscape` contract
because they synchronously inspect their inputs and retain no argument; returned
strings, metadata structs, or arrays are allocated/copied in the active context
and may themselves be arena-scoped.

The descriptor includes an annotation-source table indexed like the annotation table.
`AnnotationInfo.SourceFile`, `SourceLine`, and `SourceColumn` identify the application.
An absent source table exposes an empty file and zero coordinates. The managed
`AnnotationInfo` value layout includes these fields, so rebuild all clients.

Format 1 is the complete current layout, including all of these tables. The
runtime accepts only the current format; it does not interpret earlier layouts
that used the same version number. Regenerate generated C and metadata sidecars
and rebuild clients together when updating these contracts.

`TypeInfo` now retains a private defining-module pointer. `GetModule()` wraps that
module's descriptor, including for private declarations and closed generic types
instantiated by consumers; arrays/nullable storage belong to `cxcore`. A default or
externally fabricated type without module metadata raises a catchable exception.
This extends the `TypeInfo` value layout under the experimental ABI policy:
regenerate and rebuild all generated modules and runtime together. Native-only
`cxcore` publishes its checked-in public runtime types through the same exports.

Peer static libraries may instantiate the same closed generic type independently.
Their header layouts use a stable include guard, and the metadata name/namespace,
vtable/interface tables, and TypeInfo data use `CX_GENERIC_DATA` link-once storage
(MSVC selectany or GCC/Clang weak data). One definition is retained for the stable
closed identity. Shared module definitions keep ordinary data linkage.

`TypeInfo.GetFunction(index)` copies a function's metadata from the runtime
function table; indices match `AnnotationInfo.FunctionIndex`. `CanInvokeFunction`
reports whether a compiled invocation adapter exists. `InvokeFunction` dispatches
by index and validates the target, argument count, and runtime argument types.
Invalid indices, missing adapters, mismatched targets, and incompatible arguments
throw catchable CX exceptions. Valid void calls return null, and exceptions from
the invoked code propagate normally. `GetFunctionCount` exposes the table size.
Synchronous TypeInfo metadata queries and parameterless `CreateInstance()` are
marked `NoEscape`: they do not retain the TypeInfo receiver or other inputs.
Returned strings, arrays, metadata structs, and created objects are allocated
in the active context and can be arena-scoped.

`TypeInfo.ConvertValue(value)` preserves compatible references and exact value
types. Numeric conversion checks integral target ranges without passing through
floating point, rejects negative-to-unsigned and floating-point-to-integral conversion,
and checks finite floating-point ranges before narrowing. Bool, char, and enum
values require matching types. Converted numeric boxes retain canonical core
type descriptors rather than pointers to temporary `TypeInfo` snapshots.

An interface reference contains its dispatch vtable, receiver pointer, and an
optional owning object. For a boxed value type, the receiver points to the
boxed payload while the owner keeps the managed box available and allows casts
back to `object`. The owner is null for ordinary compiler-created class and
unboxed-struct interface references.

## Runtime types and casts

Class type information contains a base-type link and, when needed, a map from
implemented interface type information to the concrete interface vtable.
Concrete interface vtables identify the dynamic class in slot zero. Runtime
type tests and casts use these tables to recover the original object pointer
and the correct interface dispatch table.

Checked casts preserve null. A failed non-null checked cast raises
`System.InvalidCastException` at the managed boundary; type tests return false
for null. Safe casts use the separate helpers described below and return null
on failure. This checked-cast behavior is part of Milestone 24's source
migration; it does not recover from arbitrary native memory faults.

The canonical `TypeInfo` address, hash, flags, size, base-type link, and generic
arity form the read-only identity layer. Type hashes derive from the
module-qualified nested type name plus a backtick and generic arity when nonzero.
Native `cx_reflection_*` accessors expose interface, field, function, and parameter
tables and support invocation through compiled adapters. Enumerate base-class
fields through the base type's table. Static fields use
`CX_REFLECTION_NO_OFFSET`; non-virtual functions use `CX_REFLECTION_NO_SLOT`.
Metadata mutation and dynamic generic construction are not supported.

## Value representations

`cx_decimal` is a 16-byte value with three 32-bit coefficient words (`lo`,
`mid`, `hi`) and a flags word. Flags bit 31 is the sign, bits 16–23 hold a scale
from 0 to 28, and other bits are zero. Its runtime type is `System.Decimal`.
`cxruntime.h` declares arithmetic, comparison, checked conversions, parsing,
and formatting helpers, implemented in `platform/common/System/Decimal.c`.
Wide integer intermediates avoid binary floating point
in decimal arithmetic and round only once, using nearest with ties to even.

The `cx_safe_cast_*` helpers evaluate their input once and return canonical null
references on failure, including all-zero interface references. Nullable safe
unboxing copies an exact type/size match into managed nullable storage.
Generated arrays keep an immutable element-identity key in their inherited
Object `_box` slot (which is otherwise unused for array objects); this preserves
array cast identity across modules without changing the array layout.
`cx_array_with_type` attaches that key to newly generated arrays.

`typeof` consumes the existing TypeInfo ABI. `cx_typeof_array` and
`cx_typeof_nullable` construct distinct identities and retain the argument's
metadata in `GenericParams`. TypeInfo's private `RuntimeTypeInfo` pointer is
ordinary C storage so complete TypeInfo values can be copied and assigned;
the CX field remains inaccessible to consumers.

## Version checks

The public umbrella header `include/cx.h` includes the ABI version definition
from `cxdefs.h` and the `cx_runtime_abi_version()` / `cx_runtime_require_abi()`
declarations from `cxruntime.h`. `cxcore` publishes the
same number as the CMake variable `CXCORE_ABI_VERSION`. Generated projects
require ABI 1 at C compile time and reject a different CMake runtime ABI during
configuration. Generated executable entry points and dynamic module
initializers also ask the loaded runtime to verify the ABI before CX code runs.
This catches stale or mismatched dynamic runtimes at startup; a runtime that
does not export the version-check function fails to link or load.

## Versioning policy

The ABI version is an integer. While CX remains in early alpha and before its
first release, ABI changes may increment it without release compatibility
guarantees. Once compatibility expectations are established, changes that alter
generated C layout, symbol signatures, calling conventions, type identity, or
runtime behavior relied on by generated code must follow an explicit versioning
policy. The compiler's generated-code requirement and `cxcore`'s header,
runtime, and CMake version must be updated together.

ABI 1 includes a byte size in each reflection field descriptor and field flags
for raw pointers, function values, and opaque bytes.
Runtime value copying uses these descriptors to traverse managed references in
nested structs and to reject fields whose layout metadata is missing or
inconsistent. Exception frames include arena and durable-context checkpoints
so a catch transfer can unwind allocation scopes entered after the frame.
Reflected native-layout field descriptors include pointer offsets that are
checked by the native-value copy barrier; incomplete projections use a
conservative stack-only copy and cannot be published into longer-lived storage.

The current in-development ABI 1 exception layout adds a managed `_stackTrace`
reference to the base `System.Exception` layout. The CX project is unpublished;
the numeric ABI remains 1 while compiler, generated core, and dependent
artifacts are rebuilt together. This layout is not a compatibility promise for
external binaries.

The collector uses explicit native worker attachment and cooperative
collection. Native C static roots must be registered, and modules with
registered roots cannot be unloaded. Generated CX workers and `System.Threading`
APIs now use the shared runtime and cooperative safepoints;
see [threading-gc-design.md](threading-gc-design.md). Stack scan boundaries use
a separate non-inlined platform capture below the caller's complete frame,
including saved nonvolatile registers. Collection, cooperative waits, blocking
operations, and reentrant native calls share this rule; a local-variable address
is insufficient on optimized MSVC x86 builds. Keep the ABI
experimental until the supported threading, safepoints, and native interop
contracts are sufficient for a stable compatibility promise. A future stable
ABI should also record architecture, compiler/toolchain, and platform
constraints explicitly.

Native interoperability adds fixed UTF-16 wchar/WideString metadata, owned
native buffers, typed C function pointers, native loaders, callback registrations
and a reentrant native-call GC boundary. This remains a pre-release experimental
ABI: rebuild generated code and runtime together when layouts change. SDK records
use their C layout without CX headers; native functions never use managed delegate
or boxing representations. Supported foreign ABIs are MSVC Windows x86/x64 and
GCC/Clang Linux x64. Future stable compatibility changes must increment the version
and publish platform/toolchain constraints. See
[native-interop-design.md](native-interop-design.md) for the complete contract.

Task support includes generated continuations, typed `Task<T>` result storage,
by-value `ValueTask<T>`/`ValueTask` records, iterator state-machine classes, and
executor entry points under the same experimental ABI version. These layouts
are not a promise of compatibility across compiler/runtime revisions. Generated
modules, `cxcore`, and libraries such as `cxnet` must be rebuilt together. The
initial executor is cooperative and single-threaded. Its external-event API
accepts opaque contexts and provider wait functions; providers only report
readiness, and the executor dispatches callbacks on its owner thread. Socket
registrations, polling, and Winsock initialization/linkage belong to `cxnet`;
`cxcore` has no networking dependency. Native socket handles transferred into
`System.Net.Sockets.Socket` are owned by that wrapper until disposal. See
[`coroutines-design.md`](../../cxc/docs/coroutines-design.md) and
[`task-performance-design.md`](../../cxc/docs/task-performance-design.md) for
the supported async forms and allocation contracts.

## Monotonic timing

`System.Diagnostics.MonotonicClock.GetMicroseconds()` returns an unsigned elapsed-time
reading with an unspecified origin. Windows uses QueryPerformanceCounter/Frequency;
Linux uses clock_gettime(CLOCK_MONOTONIC). Readings are independent of wall-clock
adjustments, may be equal at microsecond resolution, and are not UTC timestamps.
Windows splits whole seconds before scaling to avoid intermediate overflow on long
uptimes. An unavailable OS clock throws a catchable exception.
