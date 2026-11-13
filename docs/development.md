# Runtime development

`cxcore` combines CX standard-library declarations, native implementations, and
generated metadata. Build and test instructions are in the
[README](../README.md#build-and-test-the-runtime).

## Repository layout

| Path | Contents |
| --- | --- |
| `src/System/` | Public CX APIs and implementations written in CX |
| `src/cxcore.cxproj` | The core CX library project |
| `src/*.c`, `src/*.h` | Checked-in runtime declarations and metadata fragments |
| `include/` | Public C headers; `cx.h` is the umbrella header |
| `platform/common/` | Shared native implementations and private runtime helpers |
| `platform/win/`, `platform/linux/` | OS-specific native implementations |
| `cmake/` | Configuration checks, including source-layout validation |
| `tests/` | Native runtime tests and failure-mode checks |
| `examples/` | CX applications demonstrating runtime features |
| `benchmarks/` | Performance harnesses and recorded measurements |
| `tools/` | Metadata regeneration scripts |

## Native source conventions

Implement native `extern` members in C files that mirror their CX source path.
For example, `src/System/Char.cx` maps to
`platform/common/System/Char.c`; OS-specific members belong in the matching
`platform/win` or `platform/linux` path. A CX source file declaring several types
keeps their extern implementations together in its matching C file.

Both platform trees are always included in the build. Each OS-specific C file
includes `platform/platform.h` and wraps its implementation, including OS
headers, in `#ifdef CX_PLATFORM_WIN` or `#ifdef CX_PLATFORM_LINUX`. CMake selects
platform libraries; it must not select C files or subdirectories by OS.

Shared implementations may call small OS-specific backends, as
`System/Threading/Thread.c` does for thread startup, sleep, and yield. Runtime
helpers without a CX source counterpart live directly under `platform/common`.
Private headers there are included only by the runtime build.

Some examples of implementation ownership:

- Exception handling lives in `platform/common/System/Exception.c`, with jump
  backends in the corresponding Windows and Linux `System/Exception.c` files.
- Decimal operations live in `platform/common/System/Decimal.c`.
- Task implementations and private headers mirror `src/System/Threading/Tasks/`.
- Numeric `TryParse` exports belong to each numeric type's C file; shared parsing
  helpers live in `platform/common/cxnumeric.c`.

[`CheckSourceLayout.cmake`](../cmake/CheckSourceLayout.cmake) checks matching CX
paths, source registration, platform guards, and unconditional source selection
during configuration and CTest. Its regression tests verify that invalid layouts
are rejected.

## Generated code and metadata

Keep checked-in generated implementation and metadata files under `src`.
Compiler output for the core CX project goes under `src/.obj/`. The native build
uses `src/.obj/cxcore.c` when present; otherwise, it uses the checked-in core
implementation and metadata fragments. See
[`src/CMakeLists.txt`](../src/CMakeLists.txt) for the exact selection rules.

When changing core CX declarations, update the corresponding native headers and
metadata. The scripts in `tools/` regenerate selected groups for text,
`StringBuilder`, numeric and UUID APIs, and threading/tasks. Inspect the script's
inputs and compiler path before running it; some accept a `CompilerDll` parameter.
Review generated changes alongside the CX and C implementation changes.

The generated-code ABI is currently **1**, published in `include/cxdefs.h` and
the root `CMakeLists.txt`. Keep the compiler requirement, headers, runtime checks,
and generated consumers consistent. See the [ABI contract](runtime-abi.md) before
changing layouts or calling conventions.

## Documentation

Public documentation describes supported APIs, runtime contracts, build
instructions, and current limitations. Keep internal planning, delivery tracking,
acceptance reports, and audit logs outside the source repository.

## Validation

Run the native CTest suite for changes to runtime behavior, including the
relevant failure-mode checks. Changes to CX declarations, metadata, or generated
calling conventions also need a consumer build with a matching compiler; a
native runtime build alone does not validate generated CX code.

Exercise both static and shared linking when changing exports, module metadata,
or initialization. For OS-dependent changes, validate the supported Windows and
Linux targets. Record performance measurements with the
[benchmark harness](../benchmarks/README.md), including configuration and
workload limits.
