# CX generated-code/runtime ABI

The generated C ABI is experimental. CX has not had a release and does not yet
promise binary compatibility between releases. The current ABI version is 1.
It identifies the C calling conventions, runtime entry points, runtime type
layouts, interface reference representation, reflection layouts, object vtable,
and value boxing helpers exposed through `cx.h` and generated project headers.

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
it is restricted to one OS thread and requires native C static roots to be
registered. Modules with registered roots cannot be unloaded. Keep the ABI
experimental until the supported threading, safepoints, and native interop
contracts are sufficient for a stable compatibility promise. A future stable
ABI should also record architecture, compiler/toolchain, and platform
constraints explicitly.
