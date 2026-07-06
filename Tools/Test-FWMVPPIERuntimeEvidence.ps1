param(
    [string]$OutputRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) "Saved\PIERuntimeEvidenceSynthetic\Run-$PID"),
    [switch]$KeepOutput
)

$ErrorActionPreference = "Stop"

$exporter = Join-Path $PSScriptRoot "Export-FWMVPPIERuntimeEvidence.ps1"
$syntheticLog = Join-Path $OutputRoot "FW.log"
$autoOutputRoot = $PSBoundParameters.ContainsKey("OutputRoot") -eq $false

function Invoke-Exporter {
    param(
        [switch]$ExpectFailure,
        [string]$ExpectedText
    )

    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = & powershell -NoProfile -ExecutionPolicy Bypass -File $exporter -EvidenceFolder $OutputRoot -EditorLog $syntheticLog *>&1
        $exitCode = if ($null -ne $LASTEXITCODE) { $LASTEXITCODE } else { 0 }
    }
    finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    $outputText = ($output | Out-String).Trim()
    if ($ExpectFailure) {
        if ($exitCode -eq 0) {
            Write-Host $outputText
            throw "Runtime evidence exporter was expected to fail but passed."
        }
        if ($ExpectedText -and $outputText -notmatch [regex]::Escape($ExpectedText)) {
            Write-Host $outputText
            throw "Runtime evidence exporter failure did not include expected text: $ExpectedText"
        }
        $global:LASTEXITCODE = 0
        Write-Host "[OK] Runtime evidence exporter rejected invalid synthetic log." -ForegroundColor Green
        return
    }

    if ($exitCode -ne 0) {
        Write-Host $outputText
        throw "Runtime evidence exporter was expected to pass but failed with exit code $exitCode."
    }
    $runtimeEvidence = Join-Path $OutputRoot "PIE-runtime-evidence.log"
    if (-not (Test-Path $runtimeEvidence -PathType Leaf)) {
        throw "Runtime evidence exporter did not write PIE-runtime-evidence.log."
    }
    $runtimeEvidenceText = Get-Content -Raw $runtimeEvidence
    foreach ($requiredText in @("FW MVP PIE Runtime Evidence", "UEDPIE_0_L_Test_CombatGarage", "FWCompetitiveGameMode")) {
        if ($runtimeEvidenceText -notmatch [regex]::Escape($requiredText)) {
            throw "Runtime evidence output is missing expected text: $requiredText"
        }
    }
    Write-Host "[OK] Runtime evidence exporter accepted synthetic PIE log." -ForegroundColor Green
    $global:LASTEXITCODE = 0
}

try {
    New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null
    Set-Content -Path $syntheticLog -Value @(
        "[2026.07.04-22.20.35:075][ 64]LogDebuggerCommands: Repeating last play command: Selected Viewport",
        "[2026.07.04-22.20.55:235][ 64]LogPlayLevel: Creating play world package: /Game/FW/Maps/Test/UEDPIE_0_L_Test_CombatGarage",
        "[2026.07.04-22.20.55:245][ 64]LogPlayLevel: PIE: Created PIE world by copying editor world from /Game/FW/Maps/Test/L_Test_CombatGarage.L_Test_CombatGarage to /Game/FW/Maps/Test/UEDPIE_0_L_Test_CombatGarage.L_Test_CombatGarage (0.005824s)",
        "[2026.07.04-22.20.55:759][ 64]LogLoad: Game class is 'FWCompetitiveGameMode'",
        "[2026.07.04-22.20.55:762][ 64]LogWorld: Bringing World /Game/FW/Maps/Test/UEDPIE_0_L_Test_CombatGarage.L_Test_CombatGarage up for play (max tick rate 60) at 2026.07.05-06.20.55",
        "[2026.07.04-22.20.55:764][ 64]LogWorld: Bringing up level for play took: 0.002854"
    ) -Encoding UTF8
    Invoke-Exporter

    Set-Content -Path $syntheticLog -Value "No PIE here." -Encoding UTF8
    Invoke-Exporter -ExpectFailure -ExpectedText "Editor log does not contain required PIE runtime evidence:"

    Write-Host "[OK] FW MVP PIE runtime evidence self-test completed." -ForegroundColor Green
    $global:LASTEXITCODE = 0
}
finally {
    if ($autoOutputRoot -and -not $KeepOutput -and (Test-Path $OutputRoot)) {
        Remove-Item -LiteralPath $OutputRoot -Recurse -Force
    }
}
