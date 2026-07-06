param(
    [string]$ReportPath = (Join-Path (Split-Path -Parent $PSScriptRoot) "docs\FW_MVP_PIE_Acceptance_Report.md"),
    [string]$EvidenceFolder,
    [string]$Tester = $env:USERNAME,
    [ValidateSet("Pass", "Fail")]
    [string]$OverallStatus = "Pass",
    [string]$BlockingIssues = "None",
    [string]$NonBlockingNotes = "None",
    [string]$NotesOrIssueLinks = "None",
    [switch]$MarkAllPIEItemsPassed,
    [switch]$ConfirmManualPIEComplete,
    [switch]$SkipValidation
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $ReportPath -PathType Leaf)) {
    throw "PIE acceptance report not found: $ReportPath"
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

$resolvedEvidenceFolder = (Resolve-Path -LiteralPath $EvidenceFolder).Path
$validMediaExtensions = @(".png", ".jpg", ".jpeg", ".mp4", ".mov", ".webm")
$mediaFiles = @(Get-ChildItem -LiteralPath $resolvedEvidenceFolder -Recurse -File | Where-Object {
    $_.Extension.ToLowerInvariant() -in $validMediaExtensions -and $_.Length -ge 1024
})

if ($OverallStatus -eq "Pass" -and -not $ConfirmManualPIEComplete) {
    throw "Pass finalization requires -ConfirmManualPIEComplete after manual PIE has actually been completed."
}
if ($MarkAllPIEItemsPassed -and -not $ConfirmManualPIEComplete) {
    throw "Marking all PIE items passed requires -ConfirmManualPIEComplete."
}
if ($OverallStatus -eq "Pass" -and $mediaFiles.Count -eq 0) {
    throw "Pass finalization requires at least one candidate screenshot/video file >= 1024 bytes in: $resolvedEvidenceFolder"
}

$updatedText = $reportText
$updatedText = $updatedText -replace "(?m)^- Date: .*$", "- Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
$updatedText = $updatedText -replace "(?m)^- Tester: .*$", "- Tester: $Tester"
$updatedText = $updatedText -replace "(?m)^- Screenshot or capture folder: .*$", "- Screenshot or capture folder: $resolvedEvidenceFolder"
$updatedText = $updatedText -replace "(?m)^- Notes or issue links: .*$", "- Notes or issue links: $NotesOrIssueLinks"
$updatedText = $updatedText -replace "(?m)^- Overall status: .*$", "- Overall status: $OverallStatus"
$updatedText = $updatedText -replace "(?m)^- Blocking issues: .*$", "- Blocking issues: $BlockingIssues"
$updatedText = $updatedText -replace "(?m)^- Non-blocking notes: .*$", "- Non-blocking notes: $NonBlockingNotes"

if ($MarkAllPIEItemsPassed) {
    $pieItems = [regex]::Matches($updatedText, "(?m)^\s*-\s+\[[ xX]\]\s+PIE-\d+")
    if ($pieItems.Count -ne 24) {
        throw "Expected exactly 24 PIE checklist items before marking passed, found $($pieItems.Count)."
    }
    $updatedText = $updatedText -replace "(?m)^(\s*-\s+)\[\s\](\s+PIE-\d+)", '$1[x]$2'
}

if (-not $SkipValidation) {
    $validator = Join-Path $PSScriptRoot "Test-FWMVPPIEAcceptance.ps1"
    $previewReportPath = Join-Path $resolvedEvidenceFolder ".fw-mvp-pie-acceptance-preview-$PID.md"
    try {
        Set-Content -Path $previewReportPath -Value $updatedText -Encoding UTF8
        $previousErrorActionPreference = $ErrorActionPreference
        $ErrorActionPreference = "Continue"
        try {
            & powershell -NoProfile -ExecutionPolicy Bypass -File $validator -ReportPath $previewReportPath
            $validatorExitCode = if ($null -ne $LASTEXITCODE) { $LASTEXITCODE } else { 0 }
        }
        finally {
            $ErrorActionPreference = $previousErrorActionPreference
        }
        if ($validatorExitCode -ne 0) {
            exit $validatorExitCode
        }
    }
    finally {
        if (Test-Path $previewReportPath -PathType Leaf) {
            Remove-Item -LiteralPath $previewReportPath -Force
        }
    }
}

Set-Content -Path $ReportPath -Value $updatedText -Encoding UTF8

Write-Host "[OK] FW MVP PIE acceptance report finalized: $ReportPath" -ForegroundColor Green
