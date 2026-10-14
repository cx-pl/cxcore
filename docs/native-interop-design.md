# Native C interoperability contract

Supported ABIs are MSVC Windows x86/x64 and GCC/Clang Linux x64. Generated CMake selects target OS, architecture and compiler, including cross compilation. It does not use the compiler host to infer C integer sizes. Other combinations fail native dependency configuration. Acceptance evidence is in CX_ROADMAP.md and the conformance/integration runners.

## Annotations and imports

Annotations resolve exact names through namespace imports, without suffix elision. Positional constant arguments precede named `name = value` arguments; ordinary calls retain `name: value`. Unknown, duplicate, missing, nonconstant and out-of-range arguments fail binding. User annotations derive from System.Annotation. AnnotationUsage controls targets and repeatability; parameter annotations and explicit `@return:Tag` are supported. Arguments are metadata, and constructors do not run during compilation. Runtime annotation discovery/construction as annotation metadata is a later feature.

NativeImport applies to nongeneric extern top-level functions/static methods with native signatures. Logical library ID, exact case-sensitive symbol, convention and error policy survive project and restored source-package references. Plain extern retains the CX ABI. Raw imports need no CX forwarding export. Conflicting signatures, managed arguments, instance receivers, virtual and generic imports are rejected.

SDK headers are authoritative. Generated assertions compare function-pointer signatures, field types, requested size/alignment and offsets. Pointer pointees and SDK typedefs retain exact identity. Fixed-width integers passed by value allow ABI-equivalent C integer spellings on supported targets. Explicit Cdecl/Stdcall/Winapi are checked. PlatformDefault uses an SDK function's declared convention, or Cdecl for a headerless prototype; NativeFunction PlatformDefault uses Cdecl. Windows x64 has its unified convention. Stdcall/Winapi and LastError require Windows. Function-like macros need equivalent callable SDK APIs, CX code or a separately designed header adapter; an import cannot invent an exported symbol.

captureError takes an immediate TLS snapshot of errno/GetLastError before managed cleanup; NativeError reads it. SDK-specific error APIs remain explicit. Raw calls and pointer/function-pointer operations require an unsafe function or `unsafe { ... }` block. Ordinary CX wrappers can contain that boundary.

## Types, layouts and memory

ptr is void*. ptr<T> preserves its pointee; ptr<const T> and const ptr<T> have separate meanings. Null, address-of native locals, dereference/indexing, pointer-to-pointer parameters and explicit conversions are supported. const void borrows cannot be dereferenced. Managed objects are not addressable native records. Known stack borrows cannot be returned or passed into a parked blocking native call. Other unsafe pointer validity remains the author's responsibility.

CLong/CULong, SizeT, PtrDiffT, IntPtr/UIntPtr, CBool and CChar use target C types. CX long remains 64-bit. Windows BOOL is an integer, CBool is _Bool, and GLboolean uses its SDK scalar mapping. NativeType scalar/handle declarations preserve SDK identity, particularly for pointer parameters. Explicit enum underlying types provide fixed native representations and checked constant ranges.

NativeType uses the SDK C type directly. Opaque types are legal only through pointers: no allocation, value, field access, boxing, sizeof or managed layout. SDK records/unions have checked projections and fixed inline arrays. NativeLayout emits sequential/union C storage with packing, alignment and offset assertions, without a CX object header. Definitions are ordered by dependencies; cycles of fields passed by value are diagnosed. Bitfields and flexible arrays fail C projection checks. The C compiler implements record argument/return ABI lowering.

SDK/layout declarations are namespace-level in this initial ABI; nested layouts
and managed nullable storage for native values are diagnosed. Stack storage and
parked native contexts honor record alignment. NativeBuffer.AllocateAligned accepts
an explicit power-of-two alignment for native heap storage.

sizeof, alignof and offsetof query complete native types. stackalloc<T>(count) creates caller-owned native storage, limited to 1 MiB with checked counts. NativeBuffer owns malloc storage; Allocate, Copy and explicit idempotent Dispose separate it from collectible Memory.Alloc. SDK allocations require their matching allocator/destructor. Owners are thread-confined unless their SDK/wrapper explicitly supports concurrent use; never dispose during a borrowed operation.

## Text and buffers

wchar is an unsigned 16-bit UTF-16 code unit on every target, independently of C wchar_t. char is a 32-bit scalar; a supplementary scalar requires two wchar units. Surrogates are legal individual units. Explicit numeric conversions diagnose out-of-range constants. wchar arrays/pointers and reflection metadata are present.

WideString owns an immutable GC copy. Length/Item count/index units; ScalarCount/ScalarAt validate and enumerate scalars. Content equality, hash and ordinal comparison use stored units, preserving empty strings and embedded zeros. Construction copies an explicit unit count. String.ToWideString and the virtual WideString.ToString transcode strictly and explicitly, including through Object. No normalization, implicit conversion or automatic native marshaling occurs. EncodingException.Position identifies the malformed UTF-8 byte or UTF-16 unit.

NativeBuffer.FromUtf8/FromUtf16 make owned copies. Requested terminators count in ByteLength but not managed text length. Embedded zeros survive and terminate C APIs early when those APIs ignore explicit lengths. ToUtf8String(byteLength) strictly copies output bytes; ToWideString(unitCount) copies native UTF-16 output into immutable text. Helpers do not search for terminators. File/network UTF-16 byte order remains an explicit protocol choice. Keep storage undisposed through its last retained use; managed backing pointers have no implicit rooting/pinning permission.

## Loading and callbacks

Shared interop checks and native call contexts live in `platform/common/cxinterop.c`.
Type externs mirror their CX source paths under `platform/common/System/Interop`:
`NativeBuffer.c`, `NativeCallbackRegistration.c`, and `NativeLibrary.c` own those
types. `NativeImport.c` implements NativeError, and `NativeFunction.c` implements
NativeCallback, matching the CX files that declare them. Windows and Linux files
under `platform/win/System/Interop` and `platform/linux/System/Interop` provide
native error access and loader operations.

NativeFunction delegates contain one typed C function pointer, distinct from CX closures/multicast delegates. Indirect calls check non-null functions. Symbol-only procedure values remain distinct from data pointers and cannot be called directly. Supported-target conversion validates null and WGL failure sentinels. NativeLibrary.Load/Resolve<T> report load/symbol errors and copy platform function representations through typed storage. Loaded libraries remain open until process exit.

Noncapturing static CX functions convert explicitly to guarded ABI entry points. Exceptions are caught inside callbacks and return configured ABI-valid failure results; NativeCallback.GetLastException exposes failure. Native errno/last-error and the caller's pending CX exception state are restored. Callbacks attach foreign threads or resume a parked native caller before CX entry.

NativeCallback.Register<T>(closure, owner) supports captured callbacks for APIs with a void* context parameter. Registrations root closure, optional owner and failure until native unregistration succeeds and in-flight callbacks finish. Use Function<T>() and Context together. Unregister(action) stops new entries, runs the SDK unregister action, waits cooperatively, then releases roots. Failure retains roots for retry; self-unregistration is diagnosed; repeated successful unregistration is harmless. LastException survives closing. Small native context tombstones remain until process exit to make late callbacks/stale addresses fail safely; closure payloads are freed on close. APIs without context data require separately designed retained thunks.

NativeImport(blocking = true) parks a native-only context; same-thread reentry and foreign callbacks use Milestone 16's GC protocol. Stack output borrows are rejected at this boundary; use NativeBuffer. Finite nonblocking imports/indirect calls run on the registered caller. An unannotated blocking indirect call must not wait for allocating foreign CX callbacks: express it as a blocking import or supported wrapper. Pure cx_gc_run_blocking still forbids CX reentry; cx_gc_run_native_call permits callbacks.

## Dependencies, packages and deployment

native_libraries is separate from project_references and cxpm IDs. OS entries carry architectures, toolchains, headers in required include order, include paths, definitions, SDK roots, CMake packages/imported targets, pkg-config modules/versions, static/shared selection, runtime artifacts and search paths. SDK targets/static pkg-config metadata retain transitive links. Public requirements flow through binding targets. Public imports require public dependencies; private imports may sit behind shared wrappers whose public layout/signatures do not expose SDK types.

Relative assets resolve against the declaring project; `{arch}` expands from the target. Public runtime artifacts propagate to consumer outputs; Linux native targets search $ORIGIN. Provider records os/sdk/package provenance. cxpm preserves manifests and rejects unsupported native RIDs during restore. Compiler package intake requires a matching version-1 lock and restored identities, and excludes .packages from ordinary source scans. Source asset paths map to source/ and binary/RID paths to bin/RID. Native package binaries need CX declarations in their source payload; arbitrary precompiled CX modules without declarations remain unsupported.

Declared deployment artifacts also propagate from private dependencies of a shared
wrapper, without exposing its private include/link interface. Static pkg-config
requirements resolve archives and include private transitive libraries; missing
archives fail configuration. CMake SDK imports must name the matching static or
shared targets supplied by that SDK.

The independent Windows/OpenGL/MongoDB/X11 packages and package-consuming runners are in examples/native-bindings. C varargs, C++ ABI/COM, automatic marshaling, automatic header-to-CX generation, unloading and runtime annotation reflection remain later extensions.

ABI policy: these features extend experimental ABI version 1. Apps check the runtime version. Before a stable release, publish architecture/SDK/toolchain constraints and increment the ABI version for incompatible layouts, representations or calling conventions. Regenerate text metadata with tools/update-text-metadata.ps1 whenever core CX declarations change.

MSVC x64 CRT longjmp normally adds SEH unwinding. CX implements its own catch/finally
transfers and clears the target CRT jump buffer's optional SEH frame before a CX
jump. The shared exception state and handler stack live in
`platform/common/cxexceptions.c`; `platform/win/cxexceptions.c` owns this CRT
adjustment, and `platform/linux/cxexceptions.c` uses ordinary `longjmp`.
This avoids MSVC's invalid-stack unwind failure for functions realigned for
SDK records, while preserving CX cleanup. The ABI fixture exercises 64-byte aligned
record arguments/returns and subsequent exceptions in static and DLL modes.
See [Microsoft's x64 setjmp/longjmp convention](https://learn.microsoft.com/en-us/cpp/build/x64-calling-convention?view=msvc-170#setjmplongjmp).
