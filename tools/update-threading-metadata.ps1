param(
    [string] $CompilerDll = (Join-Path $PSScriptRoot '../../cxc/src/CxCompiler/bin/Debug/net10.0/CxCompiler.dll')
)
$ErrorActionPreference = 'Stop'
$runtimeRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$workspaceRoot = (Resolve-Path (Join-Path $runtimeRoot '..')).Path
$scratch = Join-Path $workspaceRoot ".build/threading-metadata-$([guid]::NewGuid().ToString('N'))"
$sources = Join-Path $scratch 'sources'
$output = Join-Path $scratch 'generated'
New-Item -ItemType Directory -Path $sources -Force | Out-Null
foreach ($name in @('Thread', 'Mutex', 'AtomicInt')) {
    Copy-Item -LiteralPath (Join-Path $runtimeRoot "src/System/Threading/$name.cx") -Destination $sources
}
Copy-Item -LiteralPath (Join-Path $runtimeRoot 'src/System/Threading/Tasks/Task.cx') -Destination $sources
Copy-Item -LiteralPath (Join-Path $runtimeRoot 'src/System/Threading/Tasks/TaskScheduler.cx') -Destination $sources
[IO.File]::WriteAllText((Join-Path $sources 'api.cxproj'), "name: cxcore`ntype: Library`n", [Text.UTF8Encoding]::new($false))
& dotnet $CompilerDll -o $output (Join-Path $sources 'api.cxproj')
if ($LASTEXITCODE -ne 0) { throw 'Threading metadata generation failed' }
$metadata = [IO.File]::ReadAllText((Join-Path $output 'cxcore.c'))
. (Join-Path $PSScriptRoot 'ConvertTo-CoreMetadataFragment.ps1')
$metadata = ConvertTo-CoreMetadataFragment $metadata
$metadata = $metadata.Replace('#include "cxcore.internal.h"', '#include "cxcore.h"').Replace('CX_CXCORE_API', 'CX_API')
# cxcore itself uses constant producer-local metadata in either link mode;
# consumer DLL metadata is initialized only when CX_DYNAMIC_MODULE is defined.
$metadata = $metadata.Replace('#if !defined(CX_STATIC_LINK)', '#if defined(CX_DYNAMIC_MODULE)')
[IO.File]::WriteAllText((Join-Path $runtimeRoot 'src/cxthreading.metadata.c'), $metadata, [Text.UTF8Encoding]::new($false))
$generatedHeader = [IO.File]::ReadAllText((Join-Path $output 'cxcore.internal.h'))
$threadingHeaderPath = Join-Path $runtimeRoot 'src/cxthreading.h'
$threadingHeader = [IO.File]::ReadAllText($threadingHeaderPath)
$stateType = [regex]::Match(
    $generatedHeader,
    'CX_TYPE_DEF\(cxcore_System_Threading_Tasks_TaskHandleWhenAllState\)\s*\{.*?\};',
    [Text.RegularExpressions.RegexOptions]::Singleline)
if (!$stateType.Success) { throw 'No TaskHandle.WhenAll state layout was generated.' }
$statePattern = [regex]::Match(
    $threadingHeader,
    'CX_TYPE_DEF\(cxcore_System_Threading_Tasks_TaskHandleWhenAllState\)\s*\{.*?\};',
    [Text.RegularExpressions.RegexOptions]::Singleline)
if ($statePattern.Success) {
    $threadingHeader = $threadingHeader.Replace($statePattern.Value, $stateType.Value)
}

$symbols = @(
    'cxcore_System_Threading_Tasks_TaskHandle_WhenAll',
    'cxcore_System_Threading_Tasks_TaskHandle_WhenAll__2',
    'cxcore_System_Threading_Tasks_TaskHandleWhenAllState_Task___const_get',
    'cxcore_System_Threading_Tasks_TaskHandleWhenAllState___constructor',
    'cxcore_System_Threading_Tasks_TaskHandleWhenAllState_Start',
    'cxcore_System_Threading_Tasks_TaskHandleWhenAllState_RegisterChild',
    'cxcore_System_Threading_Tasks_TaskHandleWhenAllState_Complete')
$missingPrototypes = [Collections.Generic.List[string]]::new()
foreach ($symbol in $symbols) {
    $escapedSymbol = [regex]::Escape($symbol)
    $prototype = [regex]::Match(
        $generatedHeader,
        '(?ms)^extern [^;]*?' + $escapedSymbol + '\([^;]*?\);')
    if (!$prototype.Success) { throw "No generated declaration for $symbol." }
    $normalizedPrototype = ($prototype.Value -replace '[ \t]+(?=\r?\n)', '').Replace(
        'CX_CXCORE_API', 'CX_API')
    $existing = [regex]::Match(
        $threadingHeader,
        '(?ms)^extern [^;]*?' + $escapedSymbol + '\([^;]*?\);')
    if ($existing.Success) {
        $threadingHeader = $threadingHeader.Replace($existing.Value, $normalizedPrototype)
    } else {
        $missingPrototypes.Add($normalizedPrototype)
    }
}
if ($missingPrototypes.Count -gt 0) {
    $fragment = ($missingPrototypes -join "`n") + "`n`n"
    $threadingHeader = $threadingHeader.Replace(
        '#endif // __CX_THREADING_H__',
        $fragment + '#endif // __CX_THREADING_H__')
}
[IO.File]::WriteAllText($threadingHeaderPath, $threadingHeader, [Text.UTF8Encoding]::new($false))
Write-Output "Updated threading metadata from the CX declarations. Scratch files: $scratch"
