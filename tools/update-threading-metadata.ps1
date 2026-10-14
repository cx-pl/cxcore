param(
    [string] $CompilerDll = (Join-Path $PSScriptRoot '../../cxc/src/bin/Debug/net10.0/CxCompiler.dll')
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
[IO.File]::WriteAllText((Join-Path $sources 'api.cxproj'), "name: cxcore`ntype: Library`n", [Text.UTF8Encoding]::new($false))
& dotnet $CompilerDll -o $output (Join-Path $sources 'api.cxproj')
if ($LASTEXITCODE -ne 0) { throw 'Threading metadata generation failed' }
$metadata = [IO.File]::ReadAllText((Join-Path $output 'cxcore.c'))
$metadata = $metadata.Replace('#include "cxcore.internal.h"', '#include "cxcore.h"').Replace('CX_CXCORE_API', 'CX_API')
# cxcore itself uses constant producer-local metadata in either link mode;
# consumer DLL metadata is initialized only when CX_DYNAMIC_MODULE is defined.
$metadata = $metadata.Replace('#if !defined(CX_STATIC_LINK)', '#if defined(CX_DYNAMIC_MODULE)')
[IO.File]::WriteAllText((Join-Path $runtimeRoot 'src/cxthreading.metadata.c'), $metadata, [Text.UTF8Encoding]::new($false))
Write-Output "Updated threading metadata from the CX declarations. Scratch files: $scratch"
