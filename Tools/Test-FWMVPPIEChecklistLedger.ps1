param(
    [string]$OutputRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) "Saved\PIEChecklistLedgerSynthetic\Run-$PID"),
    [switch]$KeepOutput
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$setter = Join-Path $PSScriptRoot "Set-FWMVPPIEChecklistItem.ps1"
$reportPath = Join-Path $OutputRoot "synthetic-report.md"
$autoOutputRoot = $PSBoundParameters.ContainsKey("OutputRoot") -eq $false

function Invoke-Setter {
    param(
        [string]$ItemId,
        [string]$EvidenceNote,
        [switch]$ExpectFailure,
        [string]$ExpectedText
    )

    $args = @(
        "-NoProfile",
        "-ExecutionPolicy", "Bypass",
        "-File", $setter,
        "-ReportPath", $reportPath,
        "-EvidenceFolder", $OutputRoot,
        "-Tester", "CodexSynthetic",
        "-ItemId", $ItemId,
        "-EvidenceNote", $EvidenceNote
    )

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
            throw "Checklist setter was expected to fail but passed."
        }
        if ($ExpectedText -and $outputText -notmatch [regex]::Escape($ExpectedText)) {
            Write-Host $outputText
            throw "Checklist setter failure did not include expected text: $ExpectedText"
        }
        $global:LASTEXITCODE = 0
        Write-Host "[OK] Checklist setter rejected invalid synthetic input." -ForegroundColor Green
        return
    }

    if ($exitCode -ne 0) {
        Write-Host $outputText
        throw "Checklist setter was expected to pass but failed with exit code $exitCode."
    }
    Write-Host "[OK] Checklist setter recorded synthetic item." -ForegroundColor Green
    $global:LASTEXITCODE = 0
}

try {
    New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("# Synthetic FW MVP PIE Acceptance Report")
    $lines.Add("")
    $lines.Add("- Tester: CodexSynthetic")
    $lines.Add("- Screenshot or capture folder: $OutputRoot")
    $lines.Add("")
    for ($index = 0; $index -lt 24; $index++) {
        $lines.Add(("- [ ] PIE-{0:D2} Synthetic checked item." -f $index))
    }
    Set-Content -Path $reportPath -Value $lines -Encoding UTF8

    Invoke-Setter -ItemId "PIE-03" -EvidenceNote "Synthetic manual action observed."
    Invoke-Setter -ItemId "PIE-03" -EvidenceNote "Synthetic manual action rechecked."
    Invoke-Setter -ItemId "PIE-99" -EvidenceNote "Invalid item." -ExpectFailure -ExpectedText "Checklist item not found in report: PIE-99"
    Invoke-Setter -ItemId "PIE-04" -EvidenceNote " " -ExpectFailure -ExpectedText "EvidenceNote is required."

    $ledgerPath = Join-Path $OutputRoot "PIE-checklist-evidence.json"
    if (-not (Test-Path $ledgerPath -PathType Leaf)) {
        throw "Checklist evidence ledger was not written."
    }
    $ledger = Get-Content -Raw $ledgerPath | ConvertFrom-Json
    $items = @($ledger.Items)
    if ($items.Count -ne 1 -or $items[0].ItemId -ne "PIE-03") {
        throw "Checklist ledger should contain exactly one upserted PIE-03 item."
    }
    if ($items[0].EvidenceNote -ne "Synthetic manual action rechecked.") {
        throw "Checklist ledger did not upsert the latest evidence note."
    }
    if ((Select-String -Path $reportPath -Pattern "^\s*-\s+\[[xX]\]\s+PIE-03").Count -ne 1) {
        throw "Checklist setter did not check PIE-03 in the report."
    }

    Write-Host "[OK] FW MVP PIE checklist ledger self-test completed." -ForegroundColor Green
    $global:LASTEXITCODE = 0
}
finally {
    if ($autoOutputRoot -and -not $KeepOutput -and (Test-Path $OutputRoot)) {
        Remove-Item -LiteralPath $OutputRoot -Recurse -Force
    }
}
