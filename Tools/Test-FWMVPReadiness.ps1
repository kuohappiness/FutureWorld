param(
    [switch]$StrictAssets,
    [switch]$VerifyPIEValidator,
    [switch]$RequirePIEAcceptance
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$requiredFiles = @(
    "FW.uproject",
    "Source/FW/FW.Build.cs",
    "Source/FW/Assets/FWPrimaryDataAsset.h",
    "Source/FW/Events/FWEventSubsystem.h",
    "Source/FW/State/FWStateMachineComponent.h",
    "Source/FW/Combat/FWHealthComponent.h",
    "Source/FW/Combat/FWHitZoneComponent.h",
    "Source/FW/Characters/FWPlayerCharacter.h",
    "Source/FW/Characters/FWAICharacter.h",
    "Source/FW/Weapons/FWWeaponBase.h",
    "Source/FW/Weapons/FWWeaponData.h",
    "Source/FW/Weapons/FWMVPWeaponClasses.h",
    "Source/FW/Weapons/FWMVPWeaponClasses.cpp",
    "Source/FW/Vehicles/FWVehicleBase.h",
    "Source/FW/Vehicles/FWVehicleData.h",
    "Source/FW/Vehicles/FWMVPVehicleClasses.h",
    "Source/FW/Vehicles/FWMVPVehicleClasses.cpp",
    "Source/FW/Core/FWCompetitiveGameMode.h",
    "Source/FW/Core/FWPlayerController.h",
    "Source/FW/UI/FWHUDWidget.h",
    "Source/FW/UI/FWHUDWidget.cpp",
    "Source/FW/World/FWCombatGarageTestDirector.h",
    "Source/FW/Commandlets/FWMVPAssetBootstrapCommandlet.h",
    "Source/FW/Commandlets/FWMVPAssetBootstrapCommandlet.cpp",
    "Source/FW/Commandlets/FWMVPVerifyCommandlet.h",
    "Source/FW/Commandlets/FWMVPVerifyCommandlet.cpp",
    "Source/FW/Commandlets/FWMVPRuntimeVerifyCommandlet.h",
    "Source/FW/Commandlets/FWMVPRuntimeVerifyCommandlet.cpp",
    "Source/FW/Commandlets/FWMVPFullLoopVerifyCommandlet.h",
    "Source/FW/Commandlets/FWMVPFullLoopVerifyCommandlet.cpp",
    "Source/FW/Tests/FWMVPSmokeTest.cpp",
    "Tools/Run-FWMVPAssetBootstrap.ps1",
    "Tools/Run-FWMVPVerify.ps1",
    "Tools/Run-FWMVPRuntimeVerify.ps1",
    "Tools/Run-FWMVPFullLoopVerify.ps1",
    "Tools/Run-FWMVPSmokeTest.ps1",
    "Tools/Run-FWMVPPIEPreflight.ps1",
    "Tools/Test-FWMVPPIEPreflightList.ps1",
    "Tools/Test-FWMVPPIESessionGateList.ps1",
    "Tools/Open-FWMVPCombatGaragePIE.ps1",
    "Tools/Capture-FWMVPPIEEvidenceScreenshot.ps1",
    "Tools/Test-FWMVPPIEEvidenceCapture.ps1",
    "Tools/Invoke-FWMVPPIEPlayEvidenceCapture.ps1",
    "Tools/Test-FWMVPPIEPlayEvidenceCapture.ps1",
    "Tools/Invoke-FWMVPPIEWeaponEvidenceCapture.ps1",
    "Tools/Test-FWMVPPIEWeaponEvidenceCapture.ps1",
    "Tools/Invoke-FWMVPPIEMovementEvidenceCapture.ps1",
    "Tools/Test-FWMVPPIEMovementEvidenceCapture.ps1",
    "Tools/Invoke-FWMVPPIECameraEvidenceCapture.ps1",
    "Tools/Test-FWMVPPIECameraEvidenceCapture.ps1",
    "Tools/Invoke-FWMVPPIEVehicleInteractionEvidenceCapture.ps1",
    "Tools/Test-FWMVPPIEVehicleInteractionEvidenceCapture.ps1",
    "Tools/Export-FWMVPPIERuntimeEvidence.ps1",
    "Tools/Test-FWMVPPIERuntimeEvidence.ps1",
    "Tools/Set-FWMVPPIEChecklistItem.ps1",
    "Tools/Test-FWMVPPIEChecklistLedger.ps1",
    "Tools/New-FWMVPPIEAcceptanceSession.ps1",
    "Tools/Test-FWMVPPIEAcceptance.ps1",
    "Tools/Test-FWMVPPIEAcceptanceValidator.ps1",
    "Tools/Complete-FWMVPPIEAcceptanceReport.ps1",
    "Tools/Test-FWMVPPIEAcceptanceFinalizer.ps1",
    "Tools/Get-FWMVPPIEAcceptanceStatus.ps1",
    "Tools/Test-FWMVPPIEAcceptanceStatus.ps1",
    "Tools/Clear-FWMVPPIEValidatorSynthetic.ps1",
    "Tools/Test-FWMVPPIEValidatorSyntheticCleanup.ps1",
    "docs/FW_MVP_Manual_Test_Checklist_v0.1.md",
    "docs/FW_MVP_Acceptance_Matrix_v0.1.md",
    "docs/FW_MVP_PIE_Acceptance_Report.md"
)

$requiredAssetPaths = @(
    "Content/FW/Maps/Test/L_Test_CombatGarage.umap",
    "Content/FW/Input/Actions/IA_Move.uasset",
    "Content/FW/Input/Actions/IA_Look.uasset",
    "Content/FW/Input/Actions/IA_Jump.uasset",
    "Content/FW/Input/Actions/IA_Run.uasset",
    "Content/FW/Input/Actions/IA_Crouch.uasset",
    "Content/FW/Input/Actions/IA_Crawl.uasset",
    "Content/FW/Input/Actions/IA_Fire.uasset",
    "Content/FW/Input/Actions/IA_Aim.uasset",
    "Content/FW/Input/Actions/IA_Reload.uasset",
    "Content/FW/Input/Actions/IA_Interact.uasset",
    "Content/FW/Input/Actions/IA_ToggleCamera.uasset",
    "Content/FW/Input/Actions/IA_WeaponSlot1.uasset",
    "Content/FW/Input/Actions/IA_WeaponSlot2.uasset",
    "Content/FW/Input/Actions/IA_WeaponSlot3.uasset",
    "Content/FW/Input/MappingContexts/IMC_OnFoot.uasset",
    "Content/FW/Input/MappingContexts/IMC_Vehicle.uasset",
    "Content/FW/Input/MappingContexts/IMC_Menu.uasset",
    "Content/FW/Weapons/Data/DA_Weapon_Pistol.uasset",
    "Content/FW/Weapons/Data/DA_Weapon_Rifle.uasset",
    "Content/FW/Weapons/Data/DA_Weapon_Sniper.uasset",
    "Content/FW/Vehicles/Data/DA_Vehicle_Car_Balanced.uasset",
    "Content/FW/Vehicles/Data/DA_Vehicle_Motorcycle_Fast.uasset",
    "Content/FW/UI/Widgets/WBP_HUD.uasset"
)

$minimumAssetSizes = @{
    "Content/FW/Maps/Test/L_Test_CombatGarage.umap" = 15000
    "Content/FW/Input/MappingContexts/IMC_OnFoot.uasset" = 5000
    "Content/FW/Input/MappingContexts/IMC_Vehicle.uasset" = 3000
    "Content/FW/UI/Widgets/WBP_HUD.uasset" = 10000
}

$missingFiles = @()
foreach ($relativePath in $requiredFiles) {
    if (-not (Test-Path (Join-Path $repoRoot $relativePath))) {
        $missingFiles += $relativePath
    }
}

$missingAssets = @()
foreach ($relativePath in $requiredAssetPaths) {
    if (-not (Test-Path (Join-Path $repoRoot $relativePath))) {
        $missingAssets += $relativePath
    }
}

if ($missingFiles.Count -gt 0) {
    Write-Host "[FAIL] Missing required source/docs files:" -ForegroundColor Red
    $missingFiles | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
    exit 1
}

Write-Host "[OK] Required source/docs files are present." -ForegroundColor Green

if ($missingAssets.Count -gt 0) {
    $status = if ($StrictAssets) { "FAIL" } else { "WARN" }
    $color = if ($StrictAssets) { "Red" } else { "Yellow" }
    Write-Host "[$status] Missing Editor-created MVP assets:" -ForegroundColor $color
    $missingAssets | ForEach-Object { Write-Host "  - $_" -ForegroundColor $color }
    if ($StrictAssets) {
        exit 2
    }
} else {
    Write-Host "[OK] Required MVP assets are present." -ForegroundColor Green
}

$weakAssets = @()
foreach ($relativePath in $minimumAssetSizes.Keys) {
    $assetPath = Join-Path $repoRoot $relativePath
    if ((Test-Path $assetPath) -and ((Get-Item $assetPath).Length -lt $minimumAssetSizes[$relativePath])) {
        $weakAssets += "$relativePath ($((Get-Item $assetPath).Length) bytes, expected >= $($minimumAssetSizes[$relativePath]))"
    }
}

if ($weakAssets.Count -gt 0) {
    $status = if ($StrictAssets) { "FAIL" } else { "WARN" }
    $color = if ($StrictAssets) { "Red" } else { "Yellow" }
    Write-Host "[$status] MVP assets look too small for the expected bootstrap output:" -ForegroundColor $color
    $weakAssets | ForEach-Object { Write-Host "  - $_" -ForegroundColor $color }
    if ($StrictAssets) {
        exit 3
    }
} else {
    Write-Host "[OK] MVP asset size sanity checks passed." -ForegroundColor Green
}

if ($VerifyPIEValidator) {
    & (Join-Path $PSScriptRoot "Test-FWMVPPIEPreflightList.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    & (Join-Path $PSScriptRoot "Test-FWMVPPIESessionGateList.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    & (Join-Path $PSScriptRoot "Test-FWMVPPIEAcceptanceValidator.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    & (Join-Path $PSScriptRoot "Test-FWMVPPIEAcceptanceFinalizer.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    & (Join-Path $PSScriptRoot "Test-FWMVPPIEAcceptanceStatus.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    & (Join-Path $PSScriptRoot "Test-FWMVPPIEEvidenceCapture.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    & (Join-Path $PSScriptRoot "Test-FWMVPPIEPlayEvidenceCapture.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    & (Join-Path $PSScriptRoot "Test-FWMVPPIEWeaponEvidenceCapture.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    & (Join-Path $PSScriptRoot "Test-FWMVPPIEMovementEvidenceCapture.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    & (Join-Path $PSScriptRoot "Test-FWMVPPIECameraEvidenceCapture.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    & (Join-Path $PSScriptRoot "Test-FWMVPPIEVehicleInteractionEvidenceCapture.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    & (Join-Path $PSScriptRoot "Test-FWMVPPIERuntimeEvidence.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    & (Join-Path $PSScriptRoot "Test-FWMVPPIEChecklistLedger.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    & (Join-Path $PSScriptRoot "Test-FWMVPPIEValidatorSyntheticCleanup.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

if ($RequirePIEAcceptance) {
    & (Join-Path $PSScriptRoot "Test-FWMVPPIEAcceptance.ps1")
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

Write-Host "[OK] FW MVP readiness foundation check completed." -ForegroundColor Green
