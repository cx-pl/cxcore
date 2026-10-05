param(
    [string]$GradebookExecutable = (Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'examples\7. Gradebook\.bin\Release\Gradebook.exe'),
    [int]$Runs = 50,
    [string]$OutputPath = ''
)

$ErrorActionPreference = 'Stop'

if ($Runs -lt 1) {
    throw 'Runs must be at least 1.'
}

$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    throw 'Run this script from an elevated PowerShell session.'
}

$executable = (Resolve-Path $GradebookExecutable).Path
$pdb = [IO.Path]::ChangeExtension($executable, '.pdb')
if (-not (Test-Path $pdb)) {
    throw "Matching PDB not found: $pdb. Build the Gradebook Release executable with /Zi and /DEBUG:FULL first."
}

$benchmarkRoot = $PSScriptRoot
$outputDirectory = Join-Path $benchmarkRoot 'out\wpr'
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $OutputPath = Join-Path $outputDirectory "Gradebook-symbolized-$(Get-Date -Format 'yyyyMMdd-HHmmss').etl"
} else {
    $OutputPath = [IO.Path]::GetFullPath($OutputPath)
}

$wpr = Join-Path $env:WINDIR 'System32\wpr.exe'
$recordingStarted = $false
try {
    & $wpr -start CPU -filemode
    if ($LASTEXITCODE -ne 0) {
        throw "WPR start failed with exit code $LASTEXITCODE. Check that no other recording is active."
    }
    $recordingStarted = $true

    for ($run = 1; $run -le $Runs; $run++) {
        & $executable *> $null
        if ($LASTEXITCODE -ne 0) {
            throw "Gradebook failed on run $run with exit code $LASTEXITCODE."
        }
    }

    & $wpr -stop $OutputPath "CX Gradebook CPU profile, $Runs runs"
    if ($LASTEXITCODE -ne 0) {
        throw "WPR stop failed with exit code $LASTEXITCODE. The recording may still be active."
    }
    $recordingStarted = $false
    Write-Output "Trace: $OutputPath"
} finally {
    if ($recordingStarted) {
        & $wpr -stop $OutputPath "Partial CX Gradebook CPU profile, $Runs runs" | Out-Null
    }
}
