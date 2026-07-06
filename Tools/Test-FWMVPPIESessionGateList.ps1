param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [string]$ProjectFile = (Join-Path (Split-Path -Parent $PSScriptRoot) "FW.uproject")
)

$ErrorActionPreference = "Stop"

$preflight = Join-Path $PSScriptRoot "Run-FWMVPPIEPreflight.ps1"
$session = Join-Path $PSScriptRoot "New-FWMVPPIEAcceptanceSession.ps1"

function Get-GateNamesFromOutput {
    param(
        [object[]]$Output
    )

    return @($Output | ForEach-Object { [string]$_ } | Where-Object {
        $_ -match "^\[[^\]]+\]"
    } | ForEach-Object {
        if ($_ -match "^\[([^\]]+)\]") {
            $Matches[1]
        }
    } | Where-Object {
        $_ -ne "OK"
    })
}

$preflightOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $preflight -EngineRoot $EngineRoot -ProjectFile $ProjectFile -ListOnly *>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host (($preflightOutput | Out-String).Trim())
    throw "FW MVP PIE preflight list failed."
}

$sessionOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $session -EngineRoot $EngineRoot -ProjectFile $ProjectFile -ListPreflightGates *>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host (($sessionOutput | Out-String).Trim())
    throw "FW MVP PIE session gate list failed."
}

$preflightGates = Get-GateNamesFromOutput -Output $preflightOutput
$sessionGates = Get-GateNamesFromOutput -Output $sessionOutput

if ($preflightGates.Count -ne $sessionGates.Count) {
    throw "Preflight runner gate count ($($preflightGates.Count)) does not match session gate count ($($sessionGates.Count))."
}

for ($index = 0; $index -lt $preflightGates.Count; $index++) {
    if ($preflightGates[$index] -ne $sessionGates[$index]) {
        throw "Gate mismatch at index ${index}: preflight=$($preflightGates[$index]), session=$($sessionGates[$index])."
    }
}

Write-Host "[OK] FW MVP PIE preflight/session gate list self-test completed." -ForegroundColor Green
$global:LASTEXITCODE = 0
