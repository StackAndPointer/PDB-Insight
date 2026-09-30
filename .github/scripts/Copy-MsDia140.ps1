<#
.SYNOPSIS
    Copy the x64 msdia140.dll next to a built PDB Insight executable.

.DESCRIPTION
    PDB Insight loads msdia140.dll from its own directory at runtime
    (NoRegCoCreate with LOAD_WITH_ALTERED_SEARCH_PATH), so the DLL must ship
    next to "PDB Insight.exe". The DLL is not stored in the repository; it is
    located from the Visual Studio DIA SDK on the build agent instead.

.PARAMETER OutputDirectory
    Directory that receives msdia140.dll (for example x64\Release).
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory
)

$ErrorActionPreference = 'Stop'

function Get-PeMachine {
    param([string]$Path)
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    if ($bytes.Length -lt 0x40) { return 0 }
    $peOffset = [System.BitConverter]::ToInt32($bytes, 0x3C)
    if ($peOffset -le 0 -or ($peOffset + 6) -ge $bytes.Length) { return 0 }
    return [System.BitConverter]::ToUInt16($bytes, $peOffset + 4)
}

$machineAmd64 = 0x8664

$candidates = New-Object System.Collections.Generic.List[string]

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (Test-Path $vswhere) {
    $installations = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    foreach ($vsPath in $installations) {
        if ([string]::IsNullOrWhiteSpace($vsPath)) { continue }
        $candidates.Add((Join-Path $vsPath 'DIA SDK\bin\amd64\msdia140.dll'))
    }
}

if ($env:VCToolsInstallDir) {
    $candidates.Add((Join-Path $env:VCToolsInstallDir '..\..\DIA SDK\bin\amd64\msdia140.dll'))
}

foreach ($root in @(${env:ProgramFiles}, ${env:ProgramFiles(x86)})) {
    if ([string]::IsNullOrWhiteSpace($root)) { continue }
    $candidates.Add((Join-Path $root 'Microsoft Visual Studio\2022\Community\DIA SDK\bin\amd64\msdia140.dll'))
    $candidates.Add((Join-Path $root 'Microsoft Visual Studio\2022\Professional\DIA SDK\bin\amd64\msdia140.dll'))
    $candidates.Add((Join-Path $root 'Microsoft Visual Studio\2022\Enterprise\DIA SDK\bin\amd64\msdia140.dll'))
    $candidates.Add((Join-Path $root 'Microsoft Visual Studio\18\Community\DIA SDK\bin\amd64\msdia140.dll'))
}

$source = $null
foreach ($candidate in $candidates) {
    if ([string]::IsNullOrWhiteSpace($candidate)) { continue }
    if (-not (Test-Path $candidate)) { continue }
    if ((Get-PeMachine -Path $candidate) -ne $machineAmd64) {
        Write-Warning "Skipping non-x64 msdia140.dll: $candidate"
        continue
    }
    $source = (Resolve-Path $candidate).Path
    break
}

if (-not $source) {
    throw 'Unable to locate an x64 msdia140.dll in the Visual Studio DIA SDK.'
}

if (-not (Test-Path $OutputDirectory)) {
    New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
}

$destination = Join-Path $OutputDirectory 'msdia140.dll'
Copy-Item -LiteralPath $source -Destination $destination -Force

Write-Host "Copied msdia140.dll"
Write-Host "  from: $source"
Write-Host "  to:   $destination"
