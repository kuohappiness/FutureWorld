param(
    [string]$OutputRoot,
    [switch]$KeepOutput
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$autoOutputRoot = $false
if (-not $OutputRoot) {
    $OutputRoot = Join-Path $repoRoot "Saved\PIEAcceptanceValidatorSynthetic\Run-$PID"
    $autoOutputRoot = $true
}

$validator = Join-Path $PSScriptRoot "Test-FWMVPPIEAcceptance.ps1"
$sessionScript = Join-Path $PSScriptRoot "New-FWMVPPIEAcceptanceSession.ps1"
$logDir = Join-Path $OutputRoot "Logs"
$reportPath = Join-Path $OutputRoot "synthetic-report.md"
$manifestPath = Join-Path $OutputRoot "manifest.json"
$readmePath = Join-Path $OutputRoot "README.md"
$runtimeEvidencePath = Join-Path $OutputRoot "PIE-runtime-evidence.log"
$checklistLedgerPath = Join-Path $OutputRoot "PIE-checklist-evidence.json"
$screenshotPath = Join-Path $OutputRoot "synthetic.png"
$requiredGates = @(
    "Build",
    "AssetBootstrap",
    "MVPVerify",
    "RuntimeVerify",
    "FullLoopVerify",
    "SmokeTest",
    "Readiness",
    "PIELaunchDryRun"
)

function New-SyntheticChecklistLedgerItems {
    $items = @()
    for ($index = 0; $index -lt 24; $index++) {
        $itemId = "PIE-{0:D2}" -f $index
        $items += [ordered]@{
            ItemId = $itemId
            CheckedAt = (Get-Date).ToString("o")
            Tester = "CodexSynthetic"
            EvidenceNote = "Synthetic manual evidence for $itemId."
            ReportLine = "- [x] $itemId Synthetic checked item."
        }
    }
    return $items
}

function Write-SyntheticChecklistLedger {
    [ordered]@{
        UpdatedAt = (Get-Date).ToString("o")
        Tester = "CodexSynthetic"
        Items = (New-SyntheticChecklistLedgerItems)
    } | ConvertTo-Json -Depth 6 | Set-Content -Path $checklistLedgerPath -Encoding UTF8
}

function Write-SyntheticPng {
    param(
        [string]$Path
    )

    $bytes = New-Object byte[] 2048
    $signature = [byte[]](0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A)
    [Array]::Copy($signature, 0, $bytes, 0, $signature.Length)
    [System.IO.File]::WriteAllBytes($Path, $bytes)
}

function New-SyntheticEvidence {
    New-Item -ItemType Directory -Path $logDir -Force | Out-Null

    $gates = @()
    foreach ($gateName in $requiredGates) {
        $outputLog = Join-Path $logDir "$gateName-Output.log"
        Set-Content -Path $outputLog -Value "Synthetic output for $gateName" -Encoding UTF8
        $gates += [ordered]@{
            Name = $gateName
            StartedAt = (Get-Date).ToString("o")
            FinishedAt = (Get-Date).ToString("o")
            ExitCode = 0
            Status = "Passed"
            LogFile = $null
            OutputLog = $outputLog
        }
    }

    $manifest = [ordered]@{
        CreatedAt = (Get-Date).ToString("o")
        Tester = "CodexSynthetic"
        ProjectFile = (Join-Path $repoRoot "FW.uproject")
        SessionDir = $OutputRoot
        SkipPreflight = $false
        Gates = $gates
    }
    $manifest | ConvertTo-Json -Depth 6 | Set-Content -Path $manifestPath -Encoding UTF8
    Set-Content -Path $readmePath -Value "# Synthetic FW MVP PIE Acceptance Session" -Encoding UTF8
    Set-Content -Path $runtimeEvidencePath -Value @(
        "# Synthetic FW MVP PIE Runtime Evidence",
        "",
        "[2026.07.04-22.20.35:075][ 64]LogDebuggerCommands: Repeating last play command: Selected Viewport",
        "[2026.07.04-22.20.55:235][ 64]LogPlayLevel: Creating play world package: /Game/FW/Maps/Test/UEDPIE_0_L_Test_CombatGarage",
        "[2026.07.04-22.20.55:245][ 64]LogPlayLevel: PIE: Created PIE world by copying editor world from /Game/FW/Maps/Test/L_Test_CombatGarage.L_Test_CombatGarage to /Game/FW/Maps/Test/UEDPIE_0_L_Test_CombatGarage.L_Test_CombatGarage (0.005824s)",
        "[2026.07.04-22.20.55:759][ 64]LogLoad: Game class is 'FWCompetitiveGameMode'",
        "[2026.07.04-22.20.55:762][ 64]LogWorld: Bringing World /Game/FW/Maps/Test/UEDPIE_0_L_Test_CombatGarage.L_Test_CombatGarage up for play (max tick rate 60) at 2026.07.05-06.20.55"
    ) -Encoding UTF8
    Write-SyntheticChecklistLedger

    Write-SyntheticPng -Path $screenshotPath

    $reportLines = New-Object System.Collections.Generic.List[string]
    $reportLines.Add("# Synthetic FW MVP PIE Acceptance Report")
    $reportLines.Add("")
    $reportLines.Add("- Date: 2026-07-05 00:00:00")
    $reportLines.Add("- Tester: CodexSynthetic")
    $reportLines.Add("- Screenshot or capture folder: $OutputRoot")
    $reportLines.Add("- Overall status: Pass")
    $reportLines.Add("- Blocking issues: None")
    $reportLines.Add("")

    for ($index = 0; $index -lt 24; $index++) {
        $reportLines.Add(("- [x] PIE-{0:D2} Synthetic checked item." -f $index))
    }

    Set-Content -Path $reportPath -Value $reportLines -Encoding UTF8
}

function Invoke-Validator {
    param(
        [switch]$ExpectFailure,
        [string]$ExpectedText,
        [string[]]$ExpectedSuccessText = @()
    )

    $output = & powershell -NoProfile -ExecutionPolicy Bypass -File $validator -ReportPath $reportPath *>&1
    $exitCode = if ($null -ne $LASTEXITCODE) { $LASTEXITCODE } else { 0 }
    $outputText = ($output | Out-String).Trim()

    if ($ExpectFailure) {
        if ($exitCode -eq 0) {
            Write-Host $outputText
            throw "Validator was expected to fail but passed."
        }
        if ($ExpectedText -and $outputText -notmatch [regex]::Escape($ExpectedText)) {
            Write-Host $outputText
            throw "Validator failure did not include expected text: $ExpectedText"
        }
        $global:LASTEXITCODE = 0
        Write-Host "[OK] Validator rejected invalid synthetic evidence." -ForegroundColor Green
        return
    }

    if ($exitCode -ne 0) {
        Write-Host $outputText
        throw "Validator was expected to pass but failed with exit code $exitCode."
    }
    foreach ($successText in $ExpectedSuccessText) {
        if ($outputText -notmatch [regex]::Escape($successText)) {
            Write-Host $outputText
            throw "Validator success output did not include expected text: $successText"
        }
    }

    Write-Host "[OK] Validator accepted complete synthetic evidence." -ForegroundColor Green
    $global:LASTEXITCODE = 0
}

function Test-SessionFailureManifest {
    $failureRoot = Join-Path $OutputRoot "SessionFailure"
    $failureName = "FailurePath"
    $failureDir = Join-Path $failureRoot $failureName
    $missingEngineRoot = Join-Path $OutputRoot "MissingUE"

    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = & powershell -NoProfile -ExecutionPolicy Bypass -File $sessionScript -EngineRoot $missingEngineRoot -EvidenceRoot $failureRoot -SessionName $failureName -Tester "CodexSyntheticFailure" *>&1
        $exitCode = if ($null -ne $LASTEXITCODE) { $LASTEXITCODE } else { 0 }
    }
    finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }

    if ($exitCode -eq 0) {
        Write-Host (($output | Out-String).Trim())
        throw "Session failure self-test was expected to fail but passed."
    }

    $failureManifestPath = Join-Path $failureDir "manifest.json"
    if (-not (Test-Path $failureManifestPath -PathType Leaf)) {
        Write-Host (($output | Out-String).Trim())
        throw "Session failure self-test did not write manifest.json."
    }
    if (-not (Test-Path (Join-Path $failureDir "README.md") -PathType Leaf)) {
        Write-Host (($output | Out-String).Trim())
        throw "Session failure self-test did not write README.md."
    }

    $manifest = Get-Content -Raw $failureManifestPath | ConvertFrom-Json
    $buildGate = @($manifest.Gates | Where-Object { $_.Name -eq "Build" }) | Select-Object -First 1
    if (-not $buildGate) {
        throw "Session failure manifest is missing the failed Build gate."
    }
    if ($buildGate.Status -ne "Failed" -or $buildGate.ExitCode -eq 0) {
        throw "Session failure manifest did not record Build as failed."
    }
    if (-not $buildGate.OutputLog -or -not (Test-Path $buildGate.OutputLog -PathType Leaf)) {
        throw "Session failure manifest did not record a Build OutputLog."
    }
    if ((Get-Item -LiteralPath $buildGate.OutputLog).Length -eq 0) {
        throw "Session failure Build OutputLog is empty."
    }

    Write-Host "[OK] Session failure path wrote README, failed gate manifest, and output log." -ForegroundColor Green
    $global:LASTEXITCODE = 0
}

try {
New-SyntheticEvidence
Invoke-Validator -ExpectedSuccessText @("Accepted session metadata:", "Accepted media evidence:", "Accepted manifest gates:", "SHA256:")

    $launchOutputLog = Join-Path $logDir "PIELaunchDryRun-Output.log"
    Clear-Content -Path $launchOutputLog
    Invoke-Validator -ExpectFailure -ExpectedText "PIE acceptance manifest gate OutputLog is empty: PIELaunchDryRun"

    Set-Content -Path $launchOutputLog -Value "Synthetic output for PIELaunchDryRun" -Encoding UTF8
    Invoke-Validator

    Set-Content -Path $screenshotPath -Value "tiny" -Encoding UTF8
    Invoke-Validator -ExpectFailure -ExpectedText "Evidence screenshot or video files must include at least one valid PNG/JPEG/MP4/MOV/WebM file >= 1024 bytes."

    Set-Content -Path $screenshotPath -Value (("not a real png" * 120)) -Encoding UTF8
    Invoke-Validator -ExpectFailure -ExpectedText "Evidence screenshot or video files must include at least one valid PNG/JPEG/MP4/MOV/WebM file >= 1024 bytes."

    Write-SyntheticPng -Path $screenshotPath
    Invoke-Validator

    Remove-Item -LiteralPath $readmePath -Force
    Invoke-Validator -ExpectFailure -ExpectedText "Evidence directory must contain README.md from New-FWMVPPIEAcceptanceSession.ps1."
    Set-Content -Path $readmePath -Value "# Synthetic FW MVP PIE Acceptance Session" -Encoding UTF8

    Remove-Item -LiteralPath $runtimeEvidencePath -Force
    Invoke-Validator -ExpectFailure -ExpectedText "Evidence directory must contain PIE-runtime-evidence.log from Export-FWMVPPIERuntimeEvidence.ps1."
    Set-Content -Path $runtimeEvidencePath -Value @(
        "# Synthetic FW MVP PIE Runtime Evidence",
        "",
        "[2026.07.04-22.20.35:075][ 64]LogDebuggerCommands: Repeating last play command: Selected Viewport",
        "[2026.07.04-22.20.55:235][ 64]LogPlayLevel: Creating play world package: /Game/FW/Maps/Test/UEDPIE_0_L_Test_CombatGarage",
        "[2026.07.04-22.20.55:245][ 64]LogPlayLevel: PIE: Created PIE world by copying editor world from /Game/FW/Maps/Test/L_Test_CombatGarage.L_Test_CombatGarage to /Game/FW/Maps/Test/UEDPIE_0_L_Test_CombatGarage.L_Test_CombatGarage (0.005824s)",
        "[2026.07.04-22.20.55:759][ 64]LogLoad: Game class is 'FWCompetitiveGameMode'",
        "[2026.07.04-22.20.55:762][ 64]LogWorld: Bringing World /Game/FW/Maps/Test/UEDPIE_0_L_Test_CombatGarage.L_Test_CombatGarage up for play (max tick rate 60) at 2026.07.05-06.20.55"
    ) -Encoding UTF8

    Remove-Item -LiteralPath $checklistLedgerPath -Force
    Invoke-Validator -ExpectFailure -ExpectedText "Evidence directory must contain PIE-checklist-evidence.json from Set-FWMVPPIEChecklistItem.ps1."
    Write-SyntheticChecklistLedger

    $manifest = Get-Content -Raw $manifestPath | ConvertFrom-Json
    $originalProjectFile = $manifest.ProjectFile
    $manifest.ProjectFile = Join-Path $OutputRoot "Missing.uproject"
    $manifest | ConvertTo-Json -Depth 6 | Set-Content -Path $manifestPath -Encoding UTF8
    Invoke-Validator -ExpectFailure -ExpectedText "PIE acceptance session manifest ProjectFile does not exist:"
    $manifest.ProjectFile = $originalProjectFile
    $manifest | ConvertTo-Json -Depth 6 | Set-Content -Path $manifestPath -Encoding UTF8
    Invoke-Validator
    Test-SessionFailureManifest

    Write-Host "[OK] FW MVP PIE acceptance validator self-test completed." -ForegroundColor Green
    $global:LASTEXITCODE = 0
}
finally {
    if ($autoOutputRoot -and -not $KeepOutput -and (Test-Path $OutputRoot)) {
        Remove-Item -LiteralPath $OutputRoot -Recurse -Force
    }
}
