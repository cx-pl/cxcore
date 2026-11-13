param([string] $CompilerDll = (Join-Path $PSScriptRoot '../../cxc/src/CxCompiler/bin/Debug/net10.0/CxCompiler.dll'))
$ErrorActionPreference = 'Stop'
$runtimeRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$workspaceRoot = (Resolve-Path (Join-Path $runtimeRoot '..')).Path
$scratch = Join-Path $workspaceRoot ".build/text-metadata-$([guid]::NewGuid().ToString('N'))"
$sources = Join-Path $scratch 'sources'
$output = Join-Path $scratch 'generated'
New-Item -ItemType Directory -Path $sources -Force | Out-Null
foreach ($source in @('System/WChar.cx','System/WideString.cx','System/EncodingException.cx','System/Interop/NativeBuffer.cx','System/Interop/NativeLibrary.cx','System/Interop/NativeCallbackRegistration.cx')) {
    Copy-Item -LiteralPath (Join-Path $runtimeRoot "src/$source") -Destination $sources
}
[IO.File]::WriteAllText((Join-Path $sources 'api.cxproj'), "name: cxcore`ntype: Library`n", [Text.UTF8Encoding]::new($false))
& dotnet $CompilerDll -o $output (Join-Path $sources 'api.cxproj')
if ($LASTEXITCODE -ne 0) { throw 'Text metadata generation failed' }
$metadata = [IO.File]::ReadAllText((Join-Path $output 'cxcore.c'))
. (Join-Path $PSScriptRoot 'ConvertTo-CoreMetadataFragment.ps1')
$metadata = ConvertTo-CoreMetadataFragment $metadata
$metadata = $metadata.Replace('#include "cxcore.internal.h"', '#include "cxcore.h"').Replace('CX_CXCORE_API', 'CX_API')
$metadata = $metadata.Replace('#if !defined(CX_STATIC_LINK)', '#if defined(CX_DYNAMIC_MODULE)')
$metadata = $metadata.Replace('__cx_module_init_CXCORE', '__cx_text_init_CXCORE').Replace('__namespace', '__text_namespace')
$metadata = [regex]::Replace($metadata, '\bvoid\* (cxcore_System_Interop_NativeCallbackRegistration_LastException___const_get)', 'struct cxcore_System_Exception* $1')
$metadata = $metadata.Replace('CX_REFLECTION_FLAG_FUNCTION_PROPERTY_GET, CX_REFLECTION_NO_SLOT, CX_NULL, "LastException"', 'CX_REFLECTION_FLAG_FUNCTION_PROPERTY_GET, CX_REFLECTION_NO_SLOT, &cxcore_System_Exception___typeinfo, "LastException"')
[IO.File]::WriteAllText((Join-Path $runtimeRoot 'src/cxtext.metadata.c'), $metadata, [Text.UTF8Encoding]::new($false))
$header = [IO.File]::ReadAllText((Join-Path $output 'cxcore.internal.h'))
$header = $header.Replace('__CXCORE_INTERNAL_H__', '__CXTEXT_API_H__').Replace('__cx_module_init_CXCORE', '__cx_text_init_CXCORE')
$header = $header.Replace('struct void* _lastException;', 'struct cxcore_System_Exception* _lastException;')
$header = [regex]::Replace($header, '\bvoid\* (cxcore_System_Interop_NativeCallbackRegistration_LastException___const_get)', 'struct cxcore_System_Exception* $1')
$header = [regex]::Replace($header, '(?m)[ \t]+(?=\r?$)', '')
[IO.File]::WriteAllText((Join-Path $runtimeRoot 'src/cxtext.api.h'), $header, [Text.UTF8Encoding]::new($false))
Write-Output "Updated text metadata from CX declarations. Scratch files: $scratch"
