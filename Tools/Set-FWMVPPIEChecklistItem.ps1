param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern("^PIE-\d{2}$")]
    [string]$ItemId,
    [Parameter(Mandatory = $true)]
    [string]$EvidenceNote,
    [string]$ReportPath,
    [string]$EvidenceFolder,
    [string]$Tester
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $ReportPath) {
    $ReportPath = Join-Path $repoRoot "docs\FW_MVP_PIE_Acceptance_Report.md"
}

if (-not (Test-Path $ReportPath -PathType Leaf)) {
    throw "PIE acceptance report not found: $ReportPath"
}
if ([string]::IsNullOrWhiteSpace($EvidenceNote)) {
    throw "EvidenceNote is required."
}

$reportText = Get-Content -Raw $ReportPath
if (-not $EvidenceFolder) {
    if ($reportText -match "Screenshot or capture folder:\s*(.+)") {
        $EvidenceFolder = $Matches[1].Trim()
    }
}
if (-not $EvidenceFolder -or $EvidenceFolder -eq "TBD") {
    throw "EvidenceFolder is required, or the report must already contain a screenshot/capture folder."
}
if (-not (Test-Path $EvidenceFolder -PathType Container)) {
    throw "Evidence folder does not exist: $EvidenceFolder"
}
if (-not $Tester) {
    if ($reportText -match "(?m)^- Tester:\s*(.+)$") {
        $Tester = $Matches[1].Trim()
    }
    if (-not $Tester -or $Tester -eq "TBD") {
        $Tester = $env:USERNAME
    }
}

$itemPattern = "(?m)^(\s*-\s+)\[[ xX]\](\s+$([regex]::Escape($ItemId))\b.*)$"
if ($reportText -notmatch $itemPattern) {
    throw "Checklist item not found in report: $ItemId"
}

$updatedText = [regex]::Replace($reportText, $itemPattern, '$1[x]$2', 1)
Set-Content -Path $ReportPath -Value $updatedText -Encoding UTF8

$resolvedEvidenceFolder = (Resolve-Path -LiteralPath $EvidenceFolder).Path
$ledgerPath = Join-Path $resolvedEvidenceFolder "PIE-checklist-evidence.json"
$ledger = [ordered]@{
    UpdatedAt = (Get-Date).ToString("o")
    Tester = $Tester
    Items = @()
}

if (Test-Path $ledgerPath -PathType Leaf) {
    $existing = Get-Content -Raw $ledgerPath | ConvertFrom-Json
    $ledger.Items = @($existing.Items)
}

$itemLine = ([regex]::Match($updatedText, "(?m)^\s*-\s+\[[xX]\]\s+$([regex]::Escape($ItemId))\b.*$")).Value.Trim()
$remainingItems = @($ledger.Items | Where-Object { $_.ItemId -ne $ItemId })
$remainingItems += [ordered]@{
    ItemId = $ItemId
    CheckedAt = (Get-Date).ToString("o")
    Tester = $Tester
    EvidenceNote = $EvidenceNote.Trim()
    ReportLine = $itemLine
}
$ledger.Items = @($remainingItems | Sort-Object ItemId)
$ledgerJson = $ledger | ConvertTo-Json -Depth 6
$writeSucceeded = $false
for ($attempt = 1; $attempt -le 5; $attempt++) {
    try {
        Set-Content -Path $ledgerPath -Value $ledgerJson -Encoding UTF8
        $writeSucceeded = $true
        break
    }
    catch [System.IO.IOException] {
        if ($attempt -eq 5) {
            throw
        }
        Start-Sleep -Milliseconds (200 * $attempt)
    }
}
if (-not $writeSucceeded) {
    throw "Failed to write checklist evidence ledger: $ledgerPath"
}

Write-Host "[OK] Recorded FW MVP PIE checklist item: $ItemId" -ForegroundColor Green
Write-Host "Evidence ledger: $ledgerPath"
