param([string]$CompilerDll = (Join-Path $PSScriptRoot '../../cxc/src/CxCompiler/bin/Release/net10.0/CxCompiler.dll'))
$ErrorActionPreference = 'Stop'
$runtimeRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$workspaceRoot = (Resolve-Path (Join-Path $runtimeRoot '..')).Path
$scratch = Join-Path $workspaceRoot ".build/stdlib-metadata-$([guid]::NewGuid().ToString('N'))"
$sources = Join-Path $scratch 'sources'
$output = Join-Path $scratch 'generated'
New-Item -ItemType Directory -Path $sources -Force | Out-Null
$names = @('Byte','SByte','Short','UShort','Int','UInt','Long','ULong','Float','Double','Decimal','Uuid')
foreach ($name in $names) {
    Copy-Item -LiteralPath (Join-Path $runtimeRoot "src/System/$name.cx") -Destination $sources
}
[IO.File]::WriteAllText((Join-Path $sources 'api.cxproj'), "name: cxcore`ntype: Library`n")
& dotnet $CompilerDll -o $output (Join-Path $sources 'api.cxproj')
if ($LASTEXITCODE -ne 0) { throw 'Standard-library metadata generation failed.' }
$generated = [IO.File]::ReadAllText((Join-Path $output 'cxcore.c'))
# Keep producer-local metadata, whose references are valid constants in both
# static and shared cxcore. Consumer-side DLL fixups belong to generated clients.
$lines = [Collections.Generic.List[string]]::new()
$conditions = [Collections.Generic.List[bool]]::new()
foreach ($line in ($generated -split '\r?\n')) {
    if ($line -eq '#if !defined(CX_DYNAMIC_MODULE)') { $conditions.Add($true); continue }
    if ($conditions.Count -gt 0 -and $line -eq '#else') { $conditions[$conditions.Count-1] = !$conditions[$conditions.Count-1]; continue }
    if ($conditions.Count -gt 0 -and $line -eq '#endif') { $conditions.RemoveAt($conditions.Count-1); continue }
    if (!$conditions.Contains($false)) { $lines.Add($line) }
}
$generated = $lines -join "`n"
$destination = Join-Path $runtimeRoot 'src/cxcore.c'
$existing = [IO.File]::ReadAllText($destination)
foreach ($name in $names) {
    $pattern = 'CX_BEGIN_VTABLE_DEF\(cxcore_System_' + $name + '\).*?' +
        'struct cxcore_System_Reflection_TypeInfo\s+' +
        'cxcore_System_' + $name + '___typeinfo\s*=\s*\{.*?\n\};'
    $regex = [regex]::new($pattern, [Text.RegularExpressions.RegexOptions]::Singleline)
    $replacement = $regex.Match($generated)
    if (!$replacement.Success) { throw "No generated metadata found for $name." }
    if ($regex.IsMatch($existing)) {
        $existing = $regex.Replace($existing, [Text.RegularExpressions.MatchEvaluator]{ param($m) $replacement.Value }, 1)
    } elseif ($name -eq 'Decimal') {
        $existing = $existing.Replace('CX_BEGIN_VTABLE_DEF(cxcore_System_Double)',
            $replacement.Value + "`n`nCX_BEGIN_VTABLE_DEF(cxcore_System_Double)")
    } else { throw "No existing metadata found for $name." }
}
if (!$existing.Contains('CX_STRING_DEF(__name_System_Decimal')) {
    $existing = $existing.Replace('CX_STRING_DEF(__name_System_Double',
        'CX_STRING_DEF(__name_System_Decimal, "Decimal");' + "`n" + 'CX_STRING_DEF(__name_System_Double')
}
[IO.File]::WriteAllText($destination, $existing, [Text.UTF8Encoding]::new($false))
Write-Output "Updated numeric and UUID metadata. Scratch files: $scratch"
