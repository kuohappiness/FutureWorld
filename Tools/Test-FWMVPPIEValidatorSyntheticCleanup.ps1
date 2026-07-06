param(
    [string]$OutputRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) "Saved\PIEAcceptanceValidatorCleanupTest\Run-$PID")
)

$ErrorActionPreference = "Stop"

$cleanupScript = Join-Path $PSScriptRoot "Clear-FWMVPPIEValidatorSynthetic.ps1"
$runFolder = Join-Path $OutputRoot "Run-Old"
$keepFolder = Join-Path $OutputRoot "Keep-Me"

try {
    New-Item -ItemType Directory -Path $runFolder -Force | Out-Null
    New-Item -ItemType Directory -Path $keepFolder -Force | Out-Null
    Set-Content -Path (Join-Path $runFolder "synthetic.txt") -Value "delete me" -Encoding UTF8
    Set-Content -Path (Join-Path $keepFolder "keep.txt") -Value "keep me" -Encoding UTF8

    $dryRunOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $cleanupScript -SyntheticRoot $OutputRoot *>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host (($dryRunOutput | Out-String).Trim())
        throw "Synthetic cleanup dry-run failed."
    }
    if (-not (Test-Path $runFolder -PathType Container)) {
        throw "Synthetic cleanup dry-run deleted Run-Old unexpectedly."
    }

    $applyOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $cleanupScript -SyntheticRoot $OutputRoot -Apply *>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host (($applyOutput | Out-String).Trim())
        throw "Synthetic cleanup apply failed."
    }
    if (Test-Path $runFolder -PathType Container) {
        throw "Synthetic cleanup apply did not delete Run-Old."
    }
    if (-not (Test-Path $keepFolder -PathType Container)) {
        throw "Synthetic cleanup apply deleted non Run-* folder."
    }

    Write-Host "[OK] FW MVP PIE validator synthetic cleanup self-test completed." -ForegroundColor Green
    $global:LASTEXITCODE = 0
}
finally {
    if (Test-Path $OutputRoot) {
        Remove-Item -LiteralPath $OutputRoot -Recurse -Force
    }
}
