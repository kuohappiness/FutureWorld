param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [string]$ProjectFile = (Join-Path (Split-Path -Parent $PSScriptRoot) "FW.uproject"),
    [int]$TimeoutSeconds = 900,
    [int]$SmokeTimeoutSeconds = 900,
    [switch]$ListOnly
)

$ErrorActionPreference = "Stop"

$steps = @(
    [ordered]@{
        Name = "Build"
        Arguments = @("-File", (Join-Path $PSScriptRoot "Test-UnrealToolchain.ps1"), "-Build", "-EngineRoot", $EngineRoot, "-ProjectFile", $ProjectFile)
    },
    [ordered]@{
        Name = "AssetBootstrap"
        Arguments = @("-File", (Join-Path $PSScriptRoot "Run-FWMVPAssetBootstrap.ps1"), "-EngineRoot", $EngineRoot, "-ProjectFile", $ProjectFile, "-TimeoutSeconds", $TimeoutSeconds)
    },
    [ordered]@{
        Name = "MVPVerify"
        Arguments = @("-File", (Join-Path $PSScriptRoot "Run-FWMVPVerify.ps1"), "-EngineRoot", $EngineRoot, "-ProjectFile", $ProjectFile, "-TimeoutSeconds", $TimeoutSeconds)
    },
    [ordered]@{
        Name = "RuntimeVerify"
        Arguments = @("-File", (Join-Path $PSScriptRoot "Run-FWMVPRuntimeVerify.ps1"), "-EngineRoot", $EngineRoot, "-ProjectFile", $ProjectFile, "-TimeoutSeconds", $TimeoutSeconds)
    },
    [ordered]@{
        Name = "FullLoopVerify"
        Arguments = @("-File", (Join-Path $PSScriptRoot "Run-FWMVPFullLoopVerify.ps1"), "-EngineRoot", $EngineRoot, "-ProjectFile", $ProjectFile, "-TimeoutSeconds", $TimeoutSeconds)
    },
    [ordered]@{
        Name = "SmokeTest"
        Arguments = @("-File", (Join-Path $PSScriptRoot "Run-FWMVPSmokeTest.ps1"), "-EngineRoot", $EngineRoot, "-ProjectFile", $ProjectFile, "-TimeoutSeconds", $SmokeTimeoutSeconds)
    },
    [ordered]@{
        Name = "Readiness"
        Arguments = @("-File", (Join-Path $PSScriptRoot "Test-FWMVPReadiness.ps1"), "-StrictAssets", "-VerifyPIEValidator")
    },
    [ordered]@{
        Name = "PIELaunchDryRun"
        Arguments = @("-File", (Join-Path $PSScriptRoot "Open-FWMVPCombatGaragePIE.ps1"), "-EngineRoot", $EngineRoot, "-ProjectFile", $ProjectFile, "-DryRun")
    }
)

function Format-Command {
    param(
        [object[]]$Arguments
    )

    $parts = @("powershell", "-NoProfile", "-ExecutionPolicy", "Bypass") + $Arguments
    return ($parts | ForEach-Object {
        $value = [string]$_
        if ($value -match "\s") {
            return "`"$value`""
        }
        return $value
    }) -join " "
}

foreach ($step in $steps) {
    $commandText = Format-Command -Arguments $step.Arguments
    if ($ListOnly) {
        Write-Host "[$($step.Name)] $commandText" -ForegroundColor Cyan
        continue
    }

    Write-Host "[RUN] $($step.Name)" -ForegroundColor Cyan
    & powershell -NoProfile -ExecutionPolicy Bypass @($step.Arguments)
    $exitCode = if ($null -ne $LASTEXITCODE) { $LASTEXITCODE } else { 0 }
    if ($exitCode -ne 0) {
        throw "$($step.Name) failed with exit code $exitCode."
    }
}

if ($ListOnly) {
    Write-Host "[OK] FW MVP PIE preflight command list completed." -ForegroundColor Green
}
else {
    Write-Host "[OK] FW MVP PIE preflight completed." -ForegroundColor Green
}
