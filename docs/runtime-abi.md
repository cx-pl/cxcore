# CX generated-code/runtime ABI

The generated C ABI is experimental. CX does not yet promise binary
compatibility between releases. ABI version 1 identifies the current C calling
conventions, runtime entry points, runtime type layouts, interface reference
representation, and reflection layouts exposed through `cx.h` and generated
project headers.

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

The ABI version is an integer. Compatible additions that preserve existing
layouts and calling conventions may retain the ABI number. Any change that can
alter generated C layout, symbol signatures, calling convention, type identity,
or runtime behavior relied on by generated code must increment it. The compiler's
generated-code requirement and `cxcore`'s header, runtime, and CMake version must
be updated together. ABI compatibility is only supported when the generated
code's required version equals the runtime's provided version; no cross-major
compatibility is implied.

Milestone 11 has established a first collector design and implementation, but it
is restricted to one OS thread and requires native C static roots to be registered.
Modules with registered roots cannot be unloaded. Keep the ABI experimental until
the supported threading, safepoints, and native interop contracts are sufficient
for a stable compatibility promise. A future stable ABI should also record
architecture, compiler/toolchain, and platform constraints explicitly.
