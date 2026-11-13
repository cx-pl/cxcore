# cxcore

The runtime and core standard library for the CX programming language.

`cxcore` provides the types, memory management, and native services that compiled
CX programs use. It combines public CX APIs in `System` with a C runtime that
supports objects, interfaces, exceptions, reflection, threading, and native
interoperability. The CX compiler emits C and links the resulting program with
`cxcore` through CMake.

CX and its runtime are experimental. Use matching compiler and runtime revisions
and rebuild generated code when updating them; binary compatibility is not yet
guaranteed.

## What's included

| Area | APIs and runtime support |
| --- | --- |
| Core types | Objects, nullable values, boxing, numeric types including `Decimal`, `Math`, and numeric parsing |
| Text and console | UTF-8 `String`, UTF-16 `WideString`, `StringBuilder`, characters, and console I/O |
| Memory | A non-moving conservative garbage collector, scoped allocation arenas, reusable `MemoryArena`, and durable copies |
| Collections | Arrays, lists, dictionaries, stacks, queues, linked lists, and synchronous and asynchronous query APIs |
| Execution | Exceptions, OS threads, mutexes, atomic integers, tasks, cancellation, and a cooperative task scheduler |
| Reflection | Type and module metadata, annotation metadata, and invocation and construction through supported compiled adapters |
| Services | Dependency injection with transient, scoped, and singleton lifetimes and explicit disposal |
| Platform and interop | Environment access, dates and times, monotonic timing, UUIDs, native buffers, library loading, and callbacks |

Individual APIs have implementation limits. See the [runtime contracts](docs/standard-library-contracts.md)
and the topic guides below for supported behavior.

## Requirements

To build the native runtime:

- CMake **3.31 or newer**.
- A native C toolchain: **MSVC on Windows** or **GCC on Linux**.
- A CMake build tool or generator available for that toolchain.

The current runtime targets Windows x86/x64 and Linux x64. macOS is not
implemented. Linux development can also use the included
[VS Code Dev Container](.devcontainer/devcontainer.json), which supplies GCC,
CMake, and GDB.

Compiling CX applications also requires `cxc`, the CX compiler. Building `cxc`
from source requires the .NET 10 SDK; the native runtime itself has no .NET
dependency.

## Use from CX

Import the namespaces you need in your CX source:

```cx
import System;

public int Main() {
    StringBuilder message = new StringBuilder();
    message.Append("Hello from CX!");
    Console.WriteLine(message.ToString());
    return 0;
}
```

Create `app.cxproj` alongside this source. Reference the core library project
to include its CX implementations; this example assumes a sibling `cxcore`
checkout:

```yaml
name: Hello
type: Executable
project_references:
- ../cxcore/src/cxcore.cxproj
```

From the application's directory, compile against that runtime checkout:

```sh
cxc --compile --cxcore-dir ../cxcore app.cxproj
```

Alternatively, set `CXCORE_SOURCE_DIR` to the checkout path. The compiler's
generated CMake project builds and links `cxcore`, so a separate runtime build
is not needed for this workflow. Generated sources go under the application's
`.obj/` directory and binaries under `.bin/`. Generated applications link the
runtime statically by default.

The bundled [arena scopes example](examples/arena-scopes) exercises temporary
allocations, durable copies, and cleanup. With `cxc` on `PATH`, build it from
the `cxcore` directory:

```sh
cxc --compile --cxcore-dir . examples/arena-scopes/ArenaScopes.cxproj
```

On Windows with the Visual Studio generator, run
`examples/arena-scopes/.bin/Release/ArenaScopes.exe`. With a single-configuration
Linux generator, run `examples/arena-scopes/.bin/ArenaScopes`.

## Build and test the runtime

Run these commands from the `cxcore` directory.

### Windows

Use a Visual Studio Developer PowerShell with the C++ build tools installed:

```powershell
cmake -S . -B build/windows -A x64 -DCX_BUILD_STATIC=ON -DBUILD_TESTING=ON
cmake --build build/windows --config Release
ctest --test-dir build/windows -C Release --output-on-failure
```

These commands use the default Visual Studio generator. Use `-A Win32` in a
separate build directory for x86.

### Linux

```sh
cmake -S . -B .build/linux -DCMAKE_BUILD_TYPE=Release -DCX_BUILD_STATIC=ON -DBUILD_TESTING=ON
cmake --build .build/linux
ctest --test-dir .build/linux --output-on-failure
```

To use the Dev Container, open `cxcore` in VS Code and select **Dev Containers:
Reopen in Container**, then run the Linux commands in its terminal. This requires
Docker with Linux containers and the VS Code Dev Containers extension.

### Build options

| CMake setting | Purpose |
| --- | --- |
| `CX_BUILD_STATIC=ON` | Build a static runtime and propagate `CX_STATIC_LINK` to CMake consumers |
| `CX_BUILD_STATIC=OFF` | Build a shared runtime; this is the standalone default |
| `BUILD_TESTING=ON` | Build native runtime tests and register them with CTest |
| `BUILD_TESTING=OFF` | Build the runtime without test targets |

Use separate build directories for different architectures, toolchains, and
linkage modes. All CX modules in one process must share one runtime heap;
applications using dynamic CX modules must share one `cxcore` library.

CTest covers runtime types and casts, reflection, numeric operations, text,
exceptions, memory and arenas, threading, task scheduling, platform services,
and source layout. Performance workloads and their measurement instructions are
in [benchmarks/README.md](benchmarks/README.md).

## Memory and resource lifetimes

Ordinary managed allocations are reclaimed by a conservative, non-moving
mark-and-sweep collector. It coordinates registered threads at cooperative
safepoints. Native integrations that retain managed references must follow the
runtime's rooting and thread-attachment contracts.

For temporary work, `Memory.WithArena` releases allocations when its callback
ends, while `MemoryArena.Run` retains reserved chunks for reuse across calls.
References allocated in a synchronous arena scope expire when that scope ends.
Use durable copies for data that must survive it.

The collector does not run finalizers. Dispose native buffers, service providers,
and other explicitly owned resources when their lifetimes end; use `try`/`finally`
when cleanup must happen on every exit path. See the
[memory contracts](docs/standard-library-contracts.md#allocation-and-ownership)
for details.

## Documentation

- [Standard-library runtime contracts](docs/standard-library-contracts.md): API behavior, ownership, text, and platform limits.
- [Managed memory](docs/managed-memory-design.md): garbage collection, roots, and allocation.
- [Scoped allocation arenas](docs/scoped-allocation-arenas-design.md): scope lifetimes, durable copies, async boundaries, and current limits.
- [Threading and GC coordination](docs/threading-gc-design.md): worker attachment, synchronization, safepoints, and blocking calls.
- [Native interoperability](docs/native-interop-design.md): C imports, layouts, buffers, callbacks, and deployment.
- [Runtime ABI](docs/runtime-abi.md): generated-code contracts, reflection metadata, and compatibility checks.
- [Collections](docs/collections-design.md) and [collection queries](docs/collection-queries-design.md): supported containers and query operations.
- [Dependency injection](docs/dependency-injection.md): registration, activation, lifetimes, and disposal.
- [Runtime development](docs/development.md): repository layout and implementation conventions.

## License

`cxcore` is distributed under the [MIT License](LICENSE).
