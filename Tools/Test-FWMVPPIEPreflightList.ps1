param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [string]$ProjectFile = (Join-Path (Split-Path -Parent $PSScriptRoot) "FW.uproject")
)

$ErrorActionPreference = "Stop"

$preflight = Join-Path $PSScriptRoot "Run-FWMVPPIEPreflight.ps1"
$expectedOrder = @(
    "Build",
    "AssetBootstrap",
    "MVPVerify",
    "RuntimeVerify",
    "FullLoopVerify",
    "SmokeTest",
    "Readiness",
    "PIELaunchDryRun"
)

$output = & powershell -NoProfile -ExecutionPolicy Bypass -File $preflight -EngineRoot $EngineRoot -ProjectFile $ProjectFile -ListOnly *>&1
$exitCode = if ($null -ne $LASTEXITCODE) { $LASTEXITCODE } else { 0 }
if ($exitCode -ne 0) {
    Write-Host (($output | Out-String).Trim())
    throw "FW MVP PIE preflight list failed with exit code $exitCode."
}

$lines = @($output | ForEach-Object { [string]$_ } | Where-Object { $_ -match "^\[[^\]]+\]\s+" })
$gateLines = @($lines | Where-Object { $_ -notmatch "^\[OK\]" })
if ($gateLines.Count -ne $expectedOrder.Count) {
    Write-Host (($output | Out-String).Trim())
    throw "Expected $($expectedOrder.Count) preflight gate line(s), found $($gateLines.Count)."
}

for ($index = 0; $index -lt $expectedOrder.Count; $index++) {
    $expectedName = $expectedOrder[$index]
    if ($gateLines[$index] -notmatch "^\[$([regex]::Escape($expectedName))\]\s+") {
        Write-Host (($output | Out-String).Trim())
        throw "Expected preflight gate $index to be $expectedName."
    }
}

$readinessLine = $gateLines | Where-Object { $_ -match "^\[Readiness\]" } | Select-Object -First 1
if ($readinessLine -notmatch "-StrictAssets" -or $readinessLine -notmatch "-VerifyPIEValidator") {
    Write-Host (($output | Out-String).Trim())
    throw "Readiness preflight line must include -StrictAssets and -VerifyPIEValidator."
}

$launchLine = $gateLines | Where-Object { $_ -match "^\[PIELaunchDryRun\]" } | Select-Object -First 1
if ($launchLine -notmatch "-DryRun") {
    Write-Host (($output | Out-String).Trim())
    throw "PIELaunchDryRun preflight line must include -DryRun."
}

Write-Host "[OK] FW MVP PIE preflight list self-test completed." -ForegroundColor Green
$global:LASTEXITCODE = 0
