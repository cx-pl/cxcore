# CX generated-code/runtime ABI

The generated C ABI is experimental. CX has not had a release and does not yet
promise binary compatibility between releases. The current ABI version is 1.
It identifies the C calling conventions, runtime entry points, runtime type
layouts, interface reference representation, reflection layouts, object vtable,
and value boxing helpers exposed through `cx.h` and generated project headers.

Runtime ABI checks are implemented in `platform/common/cxabi.c`, and type,
boxing, and reflection helpers in `platform/common/cxtype.c`. The internal GC
coordination header is `platform/common/cxgc.h` and is private to the runtime build.

An interface reference contains its dispatch vtable, receiver pointer, and an
optional owning object. For a boxed value type, the receiver points to the
boxed payload while the owner keeps the managed box available and allows casts
back to `object`. The owner is null for ordinary compiler-created class and
unboxed-struct interface references.

## Version checks

`cx.h` defines `CX_RUNTIME_ABI_VERSION` and declares
`cx_runtime_abi_version()` / `cx_runtime_require_abi()`. `cxcore` publishes the
same number as the CMake variable `CXCORE_ABI_VERSION`. Generated projects
require ABI 1 at C compile time and reject a different CMake runtime ABI during
configuration. Generated executable entry points and dynamic module
initializers also ask the loaded runtime to verify the ABI before CX code runs.
This catches stale or mismatched dynamic runtimes at startup; a runtime that
does not export the version-check function fails to link or load.

## Versioning policy

The ABI version is an integer. While CX remains in early alpha and before its
first release, ABI changes may continue to use version 1. Once compatibility
expectations are established, changes that alter generated C layout, symbol
signatures, calling conventions, type identity, or runtime behavior relied on
by generated code must follow an explicit versioning policy. The compiler's
generated-code requirement and `cxcore`'s header, runtime, and CMake version
must be updated together.

Milestone 11 has established a first collector design and implementation, but
Milestone 16 extends it with explicit native worker attachment and cooperative
collection. Native C static roots must still be registered, and modules with
registered roots cannot be unloaded. Generated CX workers and `System.Threading`
APIs now use the shared runtime and cooperative safepoints;
see [threading-gc-design.md](threading-gc-design.md). Keep the ABI
experimental until the supported threading, safepoints, and native interop
contracts are sufficient for a stable compatibility promise. A future stable
ABI should also record architecture, compiler/toolchain, and platform
constraints explicitly.

Milestone 17 extends version 1 with fixed UTF-16 wchar/WideString metadata, owned
native buffers, typed C function pointers, native loaders, callback registrations
and a reentrant native-call GC boundary. This remains a pre-release experimental
ABI: rebuild generated code and runtime together when layouts change. SDK records
use their C layout without CX headers; native functions never use managed delegate
or boxing representations. Supported foreign ABIs are MSVC Windows x86/x64 and
GCC/Clang Linux x64. Future stable compatibility changes must increment the version
and publish platform/toolchain constraints. See
[native-interop-design.md](native-interop-design.md) for the complete contract.
