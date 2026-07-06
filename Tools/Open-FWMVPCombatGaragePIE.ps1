param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [string]$ProjectFile = (Join-Path (Split-Path -Parent $PSScriptRoot) "FW.uproject"),
    [string]$MapAsset = "/Game/FW/Maps/Test/L_Test_CombatGarage",
    [switch]$StartPIE,
    [switch]$VehicleInteractionEvidence,
    [switch]$EnableLiveCoding,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$editor = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
$mapRelativePath = ($MapAsset.TrimStart("/") -replace "/", "\") + ".umap"
$mapFile = Join-Path (Join-Path $repoRoot "Content") ($mapRelativePath -replace "^Game\\", "")

if (-not (Test-Path $editor)) {
    throw "UnrealEditor.exe not found: $editor"
}

if (-not (Test-Path $ProjectFile)) {
    throw "Project file not found: $ProjectFile"
}

if (-not (Test-Path $mapFile)) {
    throw "Map file not found for ${MapAsset}: $mapFile"
}

$arguments = @(
    "`"$ProjectFile`"",
    $MapAsset,
    "-log",
    "-nop4",
    "-NoLiveCoding"
)

if ($StartPIE) {
    $arguments += "-pie"
}

if ($VehicleInteractionEvidence) {
    $arguments += "-FWPIEVehicleInteractionEvidence"
}

if ($EnableLiveCoding) {
    $arguments = $arguments | Where-Object { $_ -ne "-NoLiveCoding" }
}

if ($DryRun) {
    Write-Host "[OK] FW MVP PIE launch preflight passed." -ForegroundColor Green
    Write-Host "Editor: $editor"
    Write-Host "Project: $ProjectFile"
    Write-Host "Map: $MapAsset"
    Write-Host "Start PIE: $([bool]$StartPIE)"
    Write-Host "Vehicle interaction evidence: $([bool]$VehicleInteractionEvidence)"
    Write-Host "Command: `"$editor`" $($arguments -join ' ')"
    exit 0
}

Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Normal | Out-Null
Write-Host "[OK] Unreal Editor launched for FW MVP PIE: $MapAsset" -ForegroundColor Green
