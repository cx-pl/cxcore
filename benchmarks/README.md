# cxcore native benchmarks

This harness measures release builds of the current `cxcore` source on Windows
for x86/x64 and shared/static linking. It includes allocation, object and array
creation/access, UTF-8 string length, type checks, interface cast/dispatch,
reflection, exception throw/catch, and a mixed runtime path. Results are CSV
with one row per sample. Two warmup passes run before timed samples.

Run from a Visual Studio Developer PowerShell or a shell with CMake and the
MSVC environment available:

```powershell
./run-benchmarks.ps1 -Architecture Both -Linkage Both -Iterations 250000 -Samples 7
```

Pass `-TypeInfoPosition RuntimeTypeInfoFirst -CxCoreSourceDirectory <path>`
to run the same matrix against an isolated source variant for the TypeInfo
layout experiment. The source path should identify the complete `cxcore`
checkout containing that variant.

The benchmark thread is pinned to logical CPU 0 to reduce migration noise.
Each run writes a CSV and a text report to `results/`. The report records the
timestamp, machine, CPU, OS, source commit, CMake version, workload sizes, and
configuration. Keep the machine on a stable power plan and avoid concurrent
CPU-heavy work when collecting results. Use medians and ranges across samples;
do not compare results from different machines or toolchains as if they were a
controlled before/after test.

These are focused runtime microbenchmarks and a synthetic mixed path, not a
substitute for profiling real CX applications. Use a Windows CPU sampling
profiler on representative CX executables before selecting optimization work.
The benchmark fixture intentionally uses a synthetic runtime type hierarchy;
the TypeInfo field-position comparison must use two complete `cxcore` builds
from otherwise identical source and run the same fixture against each.

## End-to-end CPU profile

The Gradebook example can be built with private PDB symbols and sampled with
Windows Performance Recorder. Run these commands from an elevated Visual Studio
Developer PowerShell at the repository root:

```powershell
cmake -S "examples\7. Gradebook\.obj" -B "examples\7. Gradebook\.obj\profile-build" `
  -G "Visual Studio 18 2026" -A x64 `
  "-DCXCORE_SOURCE_DIR=$PWD\cxcore" -DCX_STATIC_LINK=ON `
  "-DCMAKE_C_FLAGS_RELEASE=/O2 /Ob2 /DNDEBUG /Zi" `
  "-DCMAKE_EXE_LINKER_FLAGS_RELEASE=/DEBUG:FULL"
cmake --build "examples\7. Gradebook\.obj\profile-build" --config Release --target Gradebook
& "cxcore\benchmarks\capture-gradebook-profile.ps1" -Runs 50
```

The script requires an elevated shell and a matching `Gradebook.pdb`. It saves
the ETL under `cxcore/benchmarks/out/wpr/`; keep these system-wide traces local
unless you have checked their contents before sharing. Use WPA or Xperf with
the PDB directory on the symbol path to inspect the Gradebook process stacks.
