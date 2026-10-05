param(
    [ValidateSet('x86', 'x64', 'Both')]
    [string]$Architecture = 'Both',
    [ValidateSet('Shared', 'Static', 'Both')]
    [string]$Linkage = 'Both',
    [ValidateSet('Current', 'RuntimeTypeInfoFirst')]
    [string]$TypeInfoPosition = 'Current',
    [string]$CxCoreSourceDirectory = '',
    [int]$Iterations = 250000,
    [int]$Samples = 7
)

$ErrorActionPreference = 'Stop'
$benchmarkRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$cxcoreRoot = if ([string]::IsNullOrWhiteSpace($CxCoreSourceDirectory)) {
    Split-Path -Parent $benchmarkRoot
} else {
    (Resolve-Path $CxCoreSourceDirectory).Path
}
$resultsRoot = Join-Path $benchmarkRoot 'results'
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
$cmake = if ($null -ne $cmakeCommand) {
    $cmakeCommand.Source
} else {
    'C:\Program Files\Microsoft Visual Studio\18\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
}
if (-not (Test-Path $cmake)) { throw "CMake not found: $cmake" }
$timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$resultCsv = Join-Path $resultsRoot "cxcore-$TypeInfoPosition-$timestamp.csv"
$report = Join-Path $resultsRoot "cxcore-$TypeInfoPosition-$timestamp.txt"
New-Item -ItemType Directory -Force -Path $resultsRoot | Out-Null

$architectures = if ($Architecture -eq 'Both') { @('x64', 'x86') } else { @($Architecture) }
$linkages = if ($Linkage -eq 'Both') { @('Shared', 'Static') } else { @($Linkage) }
try {
    $cpu = (Get-CimInstance Win32_Processor | Select-Object -First 1 -ExpandProperty Name)
} catch {
    $cpu = $env:PROCESSOR_IDENTIFIER
}
try {
    $os = (Get-CimInstance Win32_OperatingSystem).Caption
} catch {
    $os = [System.Runtime.InteropServices.RuntimeInformation]::OSDescription
}
$gitHead = git -c "safe.directory=$cxcoreRoot" -C $cxcoreRoot rev-parse HEAD

@(
    "Date: $(Get-Date -Format o)"
    "Machine: $env:COMPUTERNAME"
    "CPU: $cpu"
    "OS: $os"
    "cxcore commit: $gitHead"
    "Iterations per operation/sample: $Iterations"
    "Samples per operation: $Samples"
    "Warmup passes per operation: 2 at one tenth iterations"
    "Benchmark thread affinity: logical CPU 0"
    "RuntimeTypeInfo field position: $TypeInfoPosition"
    "Configuration: Release, Visual Studio CMake generator"
    "CMake: $(& $cmake --version | Select-Object -First 1)"
) | Set-Content -Encoding utf8 $report

'typeinfo_position,linkage,arch,benchmark,sample,iterations,elapsed_ms,ns_per_operation,checksum' |
    Set-Content -Encoding utf8 $resultCsv

foreach ($arch in $architectures) {
    foreach ($link in $linkages) {
        $platform = if ($arch -eq 'x86') { 'Win32' } else { 'x64' }
        $static = if ($link -eq 'Static') { 'ON' } else { 'OFF' }
        $buildRoot = Join-Path $benchmarkRoot "out\$arch-$($link.ToLowerInvariant())"
        & $cmake -S $benchmarkRoot -B $buildRoot -G 'Visual Studio 18 2026' -A $platform `
            "-DCXCORE_SOURCE_DIR=$cxcoreRoot" "-DCX_BENCH_STATIC=$static"
        if ($LASTEXITCODE -ne 0) { throw "CMake configure failed for $arch $link" }
        & $cmake --build $buildRoot --config Release --target cxcore_benchmarks
        if ($LASTEXITCODE -ne 0) { throw "Build failed for $arch $link" }

        $cache = Get-Content (Join-Path $buildRoot 'CMakeCache.txt')
        $instanceLine = ($cache | Select-String '^CMAKE_GENERATOR_INSTANCE:INTERNAL=' |
            Select-Object -First 1).Line
        $instance = $instanceLine -replace '^CMAKE_GENERATOR_INSTANCE:INTERNAL=', ''
        $toolsets = Get-ChildItem (Join-Path $instance 'VC\Tools\MSVC') -Directory |
            Sort-Object Name -Descending
        $compilerArch = if ($arch -eq 'x86') { 'x86' } else { 'x64' }
        $cl = Join-Path $toolsets[0].FullName "bin\Hostx64\$compilerArch\cl.exe"
        $compilerVersion = (& $cl /Bv 2>&1 | Select-String 'Compiler Version' |
            Select-Object -First 1).Line
        $sdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\Include'
        $sdkVersions = Get-ChildItem $sdkRoot -Directory | Sort-Object Name -Descending
        Add-Content -Encoding utf8 $report "$arch $link toolchain: $compilerVersion; Windows SDK $($sdkVersions[0].Name)"

        $executable = Join-Path $buildRoot 'bin\Release\cxcore_benchmarks.exe'
        if (-not (Test-Path $executable)) { $executable = Join-Path $buildRoot 'bin\cxcore_benchmarks.exe' }
        $rows = & $executable --iterations $Iterations --samples $Samples
        if ($LASTEXITCODE -ne 0) { throw "Benchmark failed for $arch $link" }
        foreach ($row in ($rows | Select-Object -Skip 1)) {
            "$TypeInfoPosition,$($link.ToLowerInvariant()),$row" |
                Add-Content -Encoding utf8 $resultCsv
        }
        Add-Content -Encoding utf8 $report "Completed: $arch $link"
    }
}

Write-Output "CSV: $resultCsv"
Write-Output "Report: $report"
