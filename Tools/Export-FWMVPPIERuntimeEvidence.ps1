param(
    [string]$EvidenceFolder,
    [string]$ReportPath = (Join-Path (Split-Path -Parent $PSScriptRoot) "docs\FW_MVP_PIE_Acceptance_Report.md"),
    [string]$EditorLog = (Join-Path (Split-Path -Parent $PSScriptRoot) "Saved\Logs\FW.log"),
    [string]$OutputName = "PIE-runtime-evidence.log"
)

$ErrorActionPreference = "Stop"

if (-not $EvidenceFolder) {
    if (-not (Test-Path $ReportPath -PathType Leaf)) {
        throw "PIE acceptance report not found: $ReportPath"
    }
    $reportText = Get-Content -Raw $ReportPath
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
if (-not (Test-Path $EditorLog -PathType Leaf)) {
    throw "Editor log not found: $EditorLog"
}
if ([System.IO.Path]::GetFileName($OutputName) -ne $OutputName) {
    throw "OutputName must be a file name, not a path: $OutputName"
}

$resolvedEvidenceFolder = (Resolve-Path -LiteralPath $EvidenceFolder).Path
$outputPath = Join-Path $resolvedEvidenceFolder $OutputName
$logText = Get-Content -Raw $EditorLog

$requiredPatterns = @(
    "LogDebuggerCommands: Repeating last play command: Selected Viewport",
    "LogPlayLevel: Creating play world package: /Game/FW/Maps/Test/UEDPIE_0_L_Test_CombatGarage",
    "LogPlayLevel: PIE: Created PIE world by copying editor world from /Game/FW/Maps/Test/L_Test_CombatGarage",
    "LogLoad: Game class is 'FWCompetitiveGameMode'",
    "LogWorld: Bringing World /Game/FW/Maps/Test/UEDPIE_0_L_Test_CombatGarage"
)

$missingPatterns = @()
foreach ($pattern in $requiredPatterns) {
    if ($logText -notmatch [regex]::Escape($pattern)) {
        $missingPatterns += $pattern
    }
}
if ($missingPatterns.Count -gt 0) {
    throw "Editor log does not contain required PIE runtime evidence: $($missingPatterns -join '; ')"
}

$matchingLines = Get-Content -Path $EditorLog | Where-Object {
    $_ -match "LogDebuggerCommands: Repeating last play command" -or
    $_ -match "LogPlayLevel:" -or
    $_ -match "LogLoad: Game class is 'FWCompetitiveGameMode'" -or
    $_ -match "LogWorld: Bringing World /Game/FW/Maps/Test/UEDPIE_0_L_Test_CombatGarage" -or
    $_ -match "LogWorld: Bringing up level for play"
}

$evidenceLines = @(
    "# FW MVP PIE Runtime Evidence",
    "",
    "- Created: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')",
    "- Source log: $EditorLog",
    "- Evidence folder: $resolvedEvidenceFolder",
    "",
    "## Required Signals",
    ""
)
foreach ($pattern in $requiredPatterns) {
    $evidenceLines += "- $pattern"
}
$evidenceLines += @(
    "",
    "## Matching Log Lines",
    ""
)
$evidenceLines += $matchingLines

Set-Content -Path $outputPath -Value ($evidenceLines -join [Environment]::NewLine) -Encoding UTF8
$hash = Get-FileHash -LiteralPath $outputPath -Algorithm SHA256

Write-Host "[OK] FW MVP PIE runtime evidence exported: $outputPath" -ForegroundColor Green
Write-Host "SHA256: $($hash.Hash)"
