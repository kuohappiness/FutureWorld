param(
    [string]$OutputRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) "Saved\PIEAcceptanceFinalizerSynthetic\Run-$PID"),
    [switch]$KeepOutput
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$finalizer = Join-Path $PSScriptRoot "Complete-FWMVPPIEAcceptanceReport.ps1"
$validator = Join-Path $PSScriptRoot "Test-FWMVPPIEAcceptance.ps1"
$logDir = Join-Path $OutputRoot "Logs"
$reportPath = Join-Path $OutputRoot "synthetic-report.md"
$manifestPath = Join-Path $OutputRoot "manifest.json"
$readmePath = Join-Path $OutputRoot "README.md"
$runtimeEvidencePath = Join-Path $OutputRoot "PIE-runtime-evidence.log"
$checklistLedgerPath = Join-Path $OutputRoot "PIE-checklist-evidence.json"
$screenshotPath = Join-Path $OutputRoot "synthetic.png"
$autoOutputRoot = $PSBoundParameters.ContainsKey("OutputRoot") -eq $false

function Write-SyntheticPng {
    param([string]$Path)

    $bytes = New-Object byte[] 2048
    $signature = [byte[]](0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A)
    [Array]::Copy($signature, 0, $bytes, 0, $signature.Length)
    [System.IO.File]::WriteAllBytes($Path, $bytes)
}

function New-SyntheticEvidence {
    New-Item -ItemType Directory -Path $logDir -Force | Out-Null
    Write-SyntheticPng -Path $screenshotPath
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
    $ledgerItems = @()
    for ($index = 0; $index -lt 24; $index++) {
        $itemId = "PIE-{0:D2}" -f $index
        $ledgerItems += [ordered]@{
            ItemId = $itemId
            CheckedAt = (Get-Date).ToString("o")
            Tester = "CodexSynthetic"
            EvidenceNote = "Synthetic manual evidence for $itemId."
            ReportLine = "- [x] $itemId Synthetic checked item."
        }
    }
    [ordered]@{
        UpdatedAt = (Get-Date).ToString("o")
        Tester = "CodexSynthetic"
        Items = $ledgerItems
    } | ConvertTo-Json -Depth 6 | Set-Content -Path $checklistLedgerPath -Encoding UTF8

    $requiredGates = @("Build", "AssetBootstrap", "MVPVerify", "RuntimeVerify", "FullLoopVerify", "SmokeTest", "Readiness", "PIELaunchDryRun")
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

    [ordered]@{
        CreatedAt = (Get-Date).ToString("o")
        Tester = "CodexSynthetic"
        ProjectFile = (Join-Path $repoRoot "FW.uproject")
        SessionDir = $OutputRoot
        SkipPreflight = $false
        Gates = $gates
    } | ConvertTo-Json -Depth 6 | Set-Content -Path $manifestPath -Encoding UTF8

    $reportLines = New-Object System.Collections.Generic.List[string]
    $reportLines.Add("# Synthetic FW MVP PIE Acceptance Report")
    $reportLines.Add("")
    $reportLines.Add("- Date: TBD")
    $reportLines.Add("- Tester: TBD")
    $reportLines.Add("- Screenshot or capture folder: TBD")
    $reportLines.Add("- Notes or issue links: TBD")
    $reportLines.Add("")
    for ($index = 0; $index -lt 24; $index++) {
        $reportLines.Add(("- [ ] PIE-{0:D2} Synthetic checked item." -f $index))
    }
    $reportLines.Add("")
    $reportLines.Add("- Overall status: TBD")
    $reportLines.Add("- Blocking issues: TBD")
    $reportLines.Add("- Non-blocking notes: TBD")
    Set-Content -Path $reportPath -Value $reportLines -Encoding UTF8
}

function Invoke-Finalizer {
    param(
        [switch]$ExpectFailure,
        [string]$ExpectedText,
        [switch]$Confirm
    )

    $args = @(
        "-NoProfile",
        "-ExecutionPolicy", "Bypass",
        "-File", $finalizer,
        "-ReportPath", $reportPath,
        "-EvidenceFolder", $OutputRoot,
        "-Tester", "CodexSynthetic",
        "-MarkAllPIEItemsPassed",
        "-NotesOrIssueLinks", "None",
        "-NonBlockingNotes", "None"
    )
    if ($Confirm) {
        $args += "-ConfirmManualPIEComplete"
    }

    $previousErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = & powershell @args *>&1
        $exitCode = if ($null -ne $LASTEXITCODE) { $LASTEXITCODE } else { 0 }
    }
    finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
    $outputText = ($output | Out-String).Trim()

    if ($ExpectFailure) {
        if ($exitCode -eq 0) {
            Write-Host $outputText
            throw "Finalizer was expected to fail but passed."
        }
        if ($ExpectedText -and $outputText -notmatch [regex]::Escape($ExpectedText)) {
            Write-Host $outputText
            throw "Finalizer failure did not include expected text: $ExpectedText"
        }
        $global:LASTEXITCODE = 0
        Write-Host "[OK] Finalizer rejected unsafe synthetic acceptance." -ForegroundColor Green
        return
    }

    if ($exitCode -ne 0) {
        Write-Host $outputText
        throw "Finalizer was expected to pass but failed with exit code $exitCode."
    }

    $validation = & powershell -NoProfile -ExecutionPolicy Bypass -File $validator -ReportPath $reportPath *>&1
    $validationExitCode = if ($null -ne $LASTEXITCODE) { $LASTEXITCODE } else { 0 }
    if ($validationExitCode -ne 0) {
        Write-Host (($validation | Out-String).Trim())
        throw "Finalized synthetic report did not pass the acceptance validator."
    }

    Write-Host "[OK] Finalizer completed and validator accepted synthetic evidence." -ForegroundColor Green
    $global:LASTEXITCODE = 0
}

try {
    New-SyntheticEvidence
    Invoke-Finalizer -ExpectFailure -ExpectedText "Pass finalization requires -ConfirmManualPIEComplete"
    Invoke-Finalizer -Confirm

    Remove-Item -LiteralPath $screenshotPath -Force
    Set-Content -Path $reportPath -Value (Get-Content -Raw $reportPath).Replace("- Overall status: Pass", "- Overall status: TBD") -Encoding UTF8
    Invoke-Finalizer -Confirm -ExpectFailure -ExpectedText "Pass finalization requires at least one candidate screenshot/video file"

    Write-SyntheticPng -Path $screenshotPath
    $beforeInvalidHeaderReport = Get-Content -Raw $reportPath
    Set-Content -Path $screenshotPath -Value (("not a real png" * 120)) -Encoding UTF8
    Invoke-Finalizer -Confirm -ExpectFailure -ExpectedText "Evidence screenshot or video files must include at least one valid PNG/JPEG/MP4/MOV/WebM file"
    $afterInvalidHeaderReport = Get-Content -Raw $reportPath
    if ($beforeInvalidHeaderReport -ne $afterInvalidHeaderReport) {
        throw "Finalizer mutated the report after invalid media preview validation failed."
    }

    Write-Host "[OK] FW MVP PIE acceptance finalizer self-test completed." -ForegroundColor Green
    $global:LASTEXITCODE = 0
}
finally {
    if ($autoOutputRoot -and -not $KeepOutput -and (Test-Path $OutputRoot)) {
        Remove-Item -LiteralPath $OutputRoot -Recurse -Force
    }
}
