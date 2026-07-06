param(
    [string]$SyntheticRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) "Saved\PIEAcceptanceValidatorSynthetic"),
    [int]$OlderThanDays = 0,
    [switch]$Apply
)

$ErrorActionPreference = "Stop"

function Resolve-ExistingPath {
    param(
        [string]$Path
    )

    if (-not (Test-Path $Path)) {
        return $null
    }

    return (Resolve-Path -LiteralPath $Path).Path
}

$resolvedRoot = Resolve-ExistingPath $SyntheticRoot
if (-not $resolvedRoot) {
    Write-Host "[OK] Synthetic output root does not exist: $SyntheticRoot" -ForegroundColor Green
    exit 0
}

$cutoff = (Get-Date).AddDays(-1 * $OlderThanDays)
$allCandidates = @(Get-ChildItem -LiteralPath $resolvedRoot -Directory -ErrorAction SilentlyContinue | Where-Object {
    $_.Name -like "Run-*" -and $_.LastWriteTime -le $cutoff
})

$candidates = @()
$activeCandidates = @()
foreach ($candidate in $allCandidates) {
    if ($candidate.Name -match "^Run-(\d+)$") {
        $processId = [int]$Matches[1]
        $process = Get-Process -Id $processId -ErrorAction SilentlyContinue
        if ($process) {
            $activeCandidates += $candidate
            continue
        }
    }
    $candidates += $candidate
}

if ($activeCandidates.Count -gt 0) {
    Write-Host "[INFO] Skipping active validator synthetic Run-* folder(s):" -ForegroundColor Yellow
    foreach ($candidate in $activeCandidates) {
        Write-Host "  - $($candidate.FullName)" -ForegroundColor Yellow
    }
}

if ($candidates.Count -eq 0) {
    Write-Host "[OK] No FW MVP PIE validator synthetic Run-* folders matched cleanup criteria." -ForegroundColor Green
    exit 0
}

Write-Host "[INFO] FW MVP PIE validator synthetic cleanup candidates:" -ForegroundColor Cyan
foreach ($candidate in $candidates) {
    Write-Host "  - $($candidate.FullName)" -ForegroundColor Cyan
}

if (-not $Apply) {
    Write-Host "[DRY-RUN] Re-run with -Apply to delete the listed Run-* folders." -ForegroundColor Yellow
    exit 0
}

$rootWithSeparator = $resolvedRoot.TrimEnd("\") + "\"
foreach ($candidate in $candidates) {
    $resolvedCandidate = (Resolve-Path -LiteralPath $candidate.FullName).Path
    if (-not $resolvedCandidate.StartsWith($rootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to delete path outside synthetic root: $resolvedCandidate"
    }
    if ((Split-Path -Leaf $resolvedCandidate) -notlike "Run-*") {
        throw "Refusing to delete non Run-* folder: $resolvedCandidate"
    }
}

foreach ($candidate in $candidates) {
    Remove-Item -LiteralPath $candidate.FullName -Recurse -Force
}

Write-Host "[OK] Deleted $($candidates.Count) FW MVP PIE validator synthetic Run-* folder(s)." -ForegroundColor Green
