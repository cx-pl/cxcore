param([string]$CompilerDll = (Join-Path $PSScriptRoot '../../cxc/src/CxCompiler/bin/Release/net10.0/CxCompiler.dll'))
$ErrorActionPreference = 'Stop'
$runtimeRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$workspaceRoot = (Resolve-Path (Join-Path $runtimeRoot '..')).Path
$scratch = Join-Path $workspaceRoot ".build/string-builder-metadata-$([guid]::NewGuid().ToString('N'))"
$sources = Join-Path $scratch 'sources'
$output = Join-Path $scratch 'generated'
New-Item -ItemType Directory -Path $sources -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $runtimeRoot 'src/System/StringBuilder.cx') -Destination $sources
[IO.File]::WriteAllText((Join-Path $sources 'api.cxproj'), "name: cxcore`ntype: Library`n")
& dotnet $CompilerDll -o $output (Join-Path $sources 'api.cxproj')
if ($LASTEXITCODE -ne 0) { throw 'StringBuilder metadata generation failed.' }
$metadata = [IO.File]::ReadAllText((Join-Path $output 'cxcore.c'))
. (Join-Path $PSScriptRoot 'ConvertTo-CoreMetadataFragment.ps1')
$metadata = ConvertTo-CoreMetadataFragment $metadata
$metadata = $metadata.Replace('#include "cxcore.internal.h"', '#include "cxcore.h"').Replace('CX_CXCORE_API', 'CX_API')
$metadata = $metadata.Replace('#if !defined(CX_STATIC_LINK)', '#if defined(CX_DYNAMIC_MODULE)')
$metadata = $metadata.Replace('__cx_module_init_CXCORE', '__cx_string_builder_init_CXCORE').Replace('__namespace', '__string_builder_namespace')
[IO.File]::WriteAllText((Join-Path $runtimeRoot 'src/cxstringbuilder.metadata.c'), $metadata, [Text.UTF8Encoding]::new($false))
$header = [IO.File]::ReadAllText((Join-Path $output 'cxcore.internal.h'))
$header = $header.Replace('__CXCORE_INTERNAL_H__', '__CXSTRINGBUILDER_API_H__').Replace('__cx_module_init_CXCORE', '__cx_string_builder_init_CXCORE')
[IO.File]::WriteAllText((Join-Path $runtimeRoot 'src/cxstringbuilder.api.h'), $header, [Text.UTF8Encoding]::new($false))
Write-Output "Updated StringBuilder metadata. Scratch files: $scratch"
