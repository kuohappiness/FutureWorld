param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [string]$ProjectFile = (Join-Path (Split-Path -Parent $PSScriptRoot) "FW.uproject"),
    [string]$ReportPath = (Join-Path (Split-Path -Parent $PSScriptRoot) "docs\FW_MVP_PIE_Acceptance_Report.md"),
    [string]$MapAsset = "/Game/FW/Maps/Test/L_Test_CombatGarage",
    [string]$VehicleInteractionEvidenceFolder,
    [string]$VehicleInteractionEvidencePrefix = ("FW_MVP_PIE_VehicleInteraction_{0}" -f (Get-Date -Format "yyyyMMdd-HHmmss")),
    [ValidateSet("Car", "Motorcycle")]
    [string]$VehicleInteractionTarget = "Car",
    [string]$VehicleDriveEvidenceFolder,
    [string]$VehicleDriveEvidencePrefix = ("FW_MVP_PIE_VehicleDrive_{0}" -f (Get-Date -Format "yyyyMMdd-HHmmss")),
    [ValidateSet("Car", "Motorcycle")]
    [string]$VehicleDriveTarget = "Car",
    [string]$VehicleDestructionEvidenceFolder,
    [string]$VehicleDestructionEvidencePrefix = ("FW_MVP_PIE_VehicleDestruction_{0}" -f (Get-Date -Format "yyyyMMdd-HHmmss")),
    [ValidateSet("Car", "Motorcycle")]
    [string]$VehicleDestructionTarget = "Car",
    [string]$AIPresentationEvidenceFolder,
    [string]$AIPresentationEvidencePrefix = ("FW_MVP_PIE_AI_{0}" -f (Get-Date -Format "yyyyMMdd-HHmmss")),
    [string]$DebugPresentationEvidenceFolder,
    [string]$DebugPresentationEvidencePrefix = ("FW_MVP_PIE_Debug_{0}" -f (Get-Date -Format "yyyyMMdd-HHmmss")),
    [string]$UIActionSafetyEvidenceFolder,
    [string]$UIActionSafetyEvidencePrefix = ("FW_MVP_PIE_UIActionSafety_{0}" -f (Get-Date -Format "yyyyMMdd-HHmmss")),
    [switch]$StartPIE,
    [switch]$VehicleInteractionEvidence,
    [switch]$VehicleDriveEvidence,
    [switch]$VehicleDestructionEvidence,
    [switch]$VehicleDestructionEndMatchEvidence,
    [switch]$AIPresentationEvidence,
    [switch]$DebugPresentationEvidence,
    [switch]$UIActionSafetyEvidence,
    [switch]$NoLogWindow,
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
    "-nop4",
    "-NoLiveCoding"
)

if (-not $NoLogWindow) {
    $arguments += "-log"
}

if ($StartPIE) {
    $arguments += "-pie"
}

if ($VehicleInteractionEvidence) {
    $arguments += "-FWPIEVehicleInteractionEvidence"
    $arguments += "-FWPIEVehicleInteractionTarget=$VehicleInteractionTarget"
    if (-not $VehicleInteractionEvidenceFolder -and (Test-Path $ReportPath -PathType Leaf)) {
        $reportText = Get-Content -Raw $ReportPath
        if ($reportText -match "Screenshot or capture folder:\s*(.+)") {
            $VehicleInteractionEvidenceFolder = $Matches[1].Trim()
        }
    }
    if ($VehicleInteractionEvidenceFolder -and $VehicleInteractionEvidenceFolder -ne "TBD") {
        $arguments += "-FWPIEVehicleInteractionEvidenceFolder=`"$VehicleInteractionEvidenceFolder`""
        $arguments += "-FWPIEVehicleInteractionEvidencePrefix=`"$VehicleInteractionEvidencePrefix`""
    }
}

if ($VehicleDriveEvidence) {
    $arguments += "-FWPIEVehicleDriveEvidence"
    $arguments += "-FWPIEVehicleDriveTarget=$VehicleDriveTarget"
    if (-not $VehicleDriveEvidenceFolder -and (Test-Path $ReportPath -PathType Leaf)) {
        $reportText = Get-Content -Raw $ReportPath
        if ($reportText -match "Screenshot or capture folder:\s*(.+)") {
            $VehicleDriveEvidenceFolder = $Matches[1].Trim()
        }
    }
    if ($VehicleDriveEvidenceFolder -and $VehicleDriveEvidenceFolder -ne "TBD") {
        $arguments += "-FWPIEVehicleDriveEvidenceFolder=`"$VehicleDriveEvidenceFolder`""
        $arguments += "-FWPIEVehicleDriveEvidencePrefix=`"$VehicleDriveEvidencePrefix`""
    }
}

if ($VehicleDestructionEvidence) {
    $arguments += "-FWPIEVehicleDestructionEvidence"
    $arguments += "-FWPIEVehicleDestructionTarget=$VehicleDestructionTarget"
    if ($VehicleDestructionEndMatchEvidence) {
        $arguments += "-FWPIEVehicleDestructionEndMatchEvidence"
    }
    if (-not $VehicleDestructionEvidenceFolder -and (Test-Path $ReportPath -PathType Leaf)) {
        $reportText = Get-Content -Raw $ReportPath
        if ($reportText -match "Screenshot or capture folder:\s*(.+)") {
            $VehicleDestructionEvidenceFolder = $Matches[1].Trim()
        }
    }
    if ($VehicleDestructionEvidenceFolder -and $VehicleDestructionEvidenceFolder -ne "TBD") {
        $arguments += "-FWPIEVehicleDestructionEvidenceFolder=`"$VehicleDestructionEvidenceFolder`""
        $arguments += "-FWPIEVehicleDestructionEvidencePrefix=`"$VehicleDestructionEvidencePrefix`""
    }
}

if ($AIPresentationEvidence) {
    $arguments += "-FWPIEAIPresentationEvidence"
    if (-not $AIPresentationEvidenceFolder -and (Test-Path $ReportPath -PathType Leaf)) {
        $reportText = Get-Content -Raw $ReportPath
        if ($reportText -match "Screenshot or capture folder:\s*(.+)") {
            $AIPresentationEvidenceFolder = $Matches[1].Trim()
        }
    }
    if ($AIPresentationEvidenceFolder -and $AIPresentationEvidenceFolder -ne "TBD") {
        $arguments += "-FWPIEAIPresentationEvidenceFolder=`"$AIPresentationEvidenceFolder`""
        $arguments += "-FWPIEAIPresentationEvidencePrefix=`"$AIPresentationEvidencePrefix`""
    }
}

if ($DebugPresentationEvidence) {
    $arguments += "-FWPIEDebugPresentationEvidence"
    if (-not $DebugPresentationEvidenceFolder -and (Test-Path $ReportPath -PathType Leaf)) {
        $reportText = Get-Content -Raw $ReportPath
        if ($reportText -match "Screenshot or capture folder:\s*(.+)") {
            $DebugPresentationEvidenceFolder = $Matches[1].Trim()
        }
    }
    if ($DebugPresentationEvidenceFolder -and $DebugPresentationEvidenceFolder -ne "TBD") {
        $arguments += "-FWPIEDebugPresentationEvidenceFolder=`"$DebugPresentationEvidenceFolder`""
        $arguments += "-FWPIEDebugPresentationEvidencePrefix=`"$DebugPresentationEvidencePrefix`""
    }
}

if ($UIActionSafetyEvidence) {
    $arguments += "-FWPIEUIActionSafetyEvidence"
    if (-not $UIActionSafetyEvidenceFolder -and (Test-Path $ReportPath -PathType Leaf)) {
        $reportText = Get-Content -Raw $ReportPath
        if ($reportText -match "Screenshot or capture folder:\s*(.+)") {
            $UIActionSafetyEvidenceFolder = $Matches[1].Trim()
        }
    }
    if ($UIActionSafetyEvidenceFolder -and $UIActionSafetyEvidenceFolder -ne "TBD") {
        $arguments += "-FWPIEUIActionSafetyEvidenceFolder=`"$UIActionSafetyEvidenceFolder`""
        $arguments += "-FWPIEUIActionSafetyEvidencePrefix=`"$UIActionSafetyEvidencePrefix`""
    }
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
    Write-Host "Vehicle interaction target: $VehicleInteractionTarget"
    Write-Host "Vehicle interaction evidence folder: $VehicleInteractionEvidenceFolder"
    Write-Host "Vehicle interaction evidence prefix: $VehicleInteractionEvidencePrefix"
    Write-Host "Vehicle drive evidence: $([bool]$VehicleDriveEvidence)"
    Write-Host "Vehicle drive target: $VehicleDriveTarget"
    Write-Host "Vehicle drive evidence folder: $VehicleDriveEvidenceFolder"
    Write-Host "Vehicle drive evidence prefix: $VehicleDriveEvidencePrefix"
    Write-Host "Vehicle destruction evidence: $([bool]$VehicleDestructionEvidence)"
    Write-Host "Vehicle destruction end-match evidence: $([bool]$VehicleDestructionEndMatchEvidence)"
    Write-Host "Vehicle destruction target: $VehicleDestructionTarget"
    Write-Host "Vehicle destruction evidence folder: $VehicleDestructionEvidenceFolder"
    Write-Host "Vehicle destruction evidence prefix: $VehicleDestructionEvidencePrefix"
    Write-Host "AI presentation evidence: $([bool]$AIPresentationEvidence)"
    Write-Host "AI presentation evidence folder: $AIPresentationEvidenceFolder"
    Write-Host "AI presentation evidence prefix: $AIPresentationEvidencePrefix"
    Write-Host "Debug presentation evidence: $([bool]$DebugPresentationEvidence)"
    Write-Host "Debug presentation evidence folder: $DebugPresentationEvidenceFolder"
    Write-Host "Debug presentation evidence prefix: $DebugPresentationEvidencePrefix"
    Write-Host "UI action safety evidence: $([bool]$UIActionSafetyEvidence)"
    Write-Host "UI action safety evidence folder: $UIActionSafetyEvidenceFolder"
    Write-Host "UI action safety evidence prefix: $UIActionSafetyEvidencePrefix"
    Write-Host "Log window: $(-not [bool]$NoLogWindow)"
    Write-Host "Command: `"$editor`" $($arguments -join ' ')"
    exit 0
}

Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Normal | Out-Null
Write-Host "[OK] Unreal Editor launched for FW MVP PIE: $MapAsset" -ForegroundColor Green
