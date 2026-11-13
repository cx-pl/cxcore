# cxcore benchmarks

The native benchmark harness measures release builds of `cxcore` with static
and shared linking. Workloads include allocation, objects and arrays, UTF-8
text, casts and interface dispatch, reflection, exceptions, arena ownership
checks, and synthetic request processing.

## Run on Windows

Use a Visual Studio Developer PowerShell with CMake and MSVC available.
From the `cxcore/benchmarks` directory, run:

```powershell
./run-benchmarks.ps1 -Architecture Both -Linkage Both -Iterations 250000 -Samples 7
```

`Architecture` accepts `x86`, `x64`, or `Both`. `Linkage` accepts `Shared`,
`Static`, or `Both`.

To compare a separate source variant, pass `-CxCoreSourceDirectory <path>`.
The `-TypeInfoPosition RuntimeTypeInfoFirst` label identifies a corresponding
TypeInfo-layout source variant; the source checkout must actually contain
that variant.

## Run on Linux

From the `cxcore/benchmarks` directory, run:

```sh
bash ./run-benchmarks-linux.sh 10000 5
```

The arguments are iterations per sample and sample count. The script builds
Linux x64 Release with both static and shared linking. Set `CMAKE_BIN` when
the desired CMake executable is not the default.

Both runners require CMake 3.31 or newer. They write CSV samples and an
environment report under `results/`.

## Interpret the output

Two warmup passes run before timed samples. Reports record machine, CPU, OS,
source revision, toolchain, configuration, and workload sizes. The Windows
runner pins the benchmark thread to logical CPU 0; the Linux runner uses the
first CPU allowed by the process CPU set.

Use medians and ranges across samples. Keep power settings stable and avoid
concurrent CPU-heavy work. Results from different machines, toolchains,
workloads, and runtime revisions are not a controlled before/after comparison.

Request rows can include:

- Per-sample managed collection and collected-byte deltas.
- Live managed-heap bytes after the operation.
- Arena peak use, reserved capacity, growth, and resets.
- Bytes copied into durable storage.
- Per-request p50, p95, p99, and maximum latency.
- Cumulative stop-the-world collection time.

The collection interval includes waiting for registered threads to park plus
mark/sweep work. It excludes preflight collection bookkeeping. The CSV records
an aggregate interval per sample, rather than per-collection pause percentiles.
Individual request latencies include pauses encountered by those requests.

Counters use benchmark-only instrumentation. Timing instrumentation also has
cost: request samples include clock reads and sample-array writes. Batch-average
time and per-request percentiles answer different questions.

## Arena and request workloads

The harness compares allocation/reset behavior, fixed-capacity destruction,
public arena wrappers, checked stores and copies, and heap versus arena request
graphs. Cold chunk growth and warmed reuse should be measured separately.

`synthetic_request_heap` and `synthetic_request_arena` parse a fixed
32-scalar input, build scratch arrays, and copy a scalar-only response into
durable storage. They are C-harness workloads.

`cx_runtime_request_heap` and `cx_runtime_request_arena` also run in the C
harness, using CX memory, wide-string conversion, and string-builder APIs.
The expected response is `count=3;sum=62`. These cases exercise runtime paths
without compiling a CX parser.

Fewer managed allocations do not by themselves establish lower latency.
Measure the allocation, ownership-check, durable-copy, cleanup, and collection
costs for the workload that matters.

## Compiled CX JSON request

[`examples/arena-json-request`](../examples/arena-json-request) is a separate
CX executable. It parses a fixed-schema JSON object containing 100 positive
integers, validates and transforms them, and serializes
`{"count":100,"sum":10100}`. The heap and arena paths use the same parser
and serializer; the arena path constructs its response in a durable context.

The grammar accepts JSON whitespace but only this fixed object shape. It is
a synthetic workload, rather than a general JSON library or production parser.

With `cxc` on `PATH`, build it from the `cxcore` directory:

```sh
cxc --compile --cxcore-dir . examples/arena-json-request/ArenaJsonRequest.cxproj
```

With a sibling compiler source checkout, the equivalent PowerShell command is:

```powershell
dotnet run --project ../cxc/src/CxCompiler/CxCompiler.csproj -- --compile --cxcore-dir . examples/arena-json-request/ArenaJsonRequest.cxproj
```

With the Windows Visual Studio generator, run
`examples/arena-json-request/.bin/Release/ArenaJsonRequest.exe`. With a
single-configuration Linux generator, run
`examples/arena-json-request/.bin/ArenaJsonRequest`.

The executable checks invalid input before timing, warms each strategy, and
alternates repeated batches. Forced collections between batches are outside
the timed interval. Its batch averages do not provide per-request percentiles
or separate GC pause attribution.

## Coroutine workloads

The compiler's native coroutine runner compares generated sequence consumption
and completed/pending awaits with handwritten C state machines. From the
workspace root in a Windows developer shell, run:

```powershell
./cxc/tests/native/run-coroutine-benchmarks.ps1 -Cxc ./cxc/src/CxCompiler/bin/Debug/net10.0/CxCompiler.dll
```

It reports frame bytes, managed allocations, collections, and release time.
A stack-only C iterator is a lower bound and does not model task or scheduler
costs.

## CPU profiling

The Gradebook example can be built with private PDB symbols and sampled with
Windows Performance Recorder. Generate the example's native project first,
then run from an elevated Visual Studio Developer PowerShell at the workspace
root:

```powershell
cmake -S "examples/07-gradebook/.obj" -B "examples/07-gradebook/.obj/profile-build" -A x64 "-DCXCORE_SOURCE_DIR=$PWD/cxcore" -DCX_STATIC_LINK=ON "-DCMAKE_C_FLAGS_RELEASE=/O2 /Ob2 /DNDEBUG /Zi" "-DCMAKE_EXE_LINKER_FLAGS_RELEASE=/DEBUG:FULL"
cmake --build "examples/07-gradebook/.obj/profile-build" --config Release --target Gradebook
& "cxcore/benchmarks/capture-gradebook-profile.ps1" -Runs 50
```

The capture script requires a matching `Gradebook.pdb` and saves traces under
`out/wpr/`. Inspect process stacks with WPA or Xperf and the PDB directory on
the symbol path. Keep system-wide traces local until their contents have
been reviewed.

Keep internal performance targets, acceptance reports, rollout comparisons,
and raw diagnostic captures outside the source repository.
