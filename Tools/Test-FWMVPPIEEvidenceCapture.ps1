param(
    [string]$OutputRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) "Saved\PIEEvidenceCaptureSynthetic\Run-$PID"),
    [switch]$KeepOutput
)

$ErrorActionPreference = "Stop"

$captureScript = Join-Path $PSScriptRoot "Capture-FWMVPPIEEvidenceScreenshot.ps1"
$reportPath = Join-Path $OutputRoot "synthetic-report.md"
$autoOutputRoot = $PSBoundParameters.ContainsKey("OutputRoot") -eq $false

function Invoke-CaptureDryRun {
    param(
        [switch]$ExpectFailure,
        [string]$ExpectedText,
        [string[]]$ExtraArgs = @()
    )

    $args = @(
        "-NoProfile",
        "-ExecutionPolicy", "Bypass",
        "-File", $captureScript,
        "-ReportPath", $reportPath,
        "-DryRun"
    ) + $ExtraArgs

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
            throw "Capture dry-run was expected to fail but passed."
        }
        if ($ExpectedText -and $outputText -notmatch [regex]::Escape($ExpectedText)) {
            Write-Host $outputText
            throw "Capture dry-run failure did not include expected text: $ExpectedText"
        }
        $global:LASTEXITCODE = 0
        Write-Host "[OK] Screenshot capture rejected unsafe synthetic input." -ForegroundColor Green
        return
    }

    if ($exitCode -ne 0) {
        Write-Host $outputText
        throw "Capture dry-run was expected to pass but failed with exit code $exitCode."
    }
    if ($ExpectedText -and $outputText -notmatch [regex]::Escape($ExpectedText)) {
        Write-Host $outputText
        throw "Capture dry-run output did not include expected text: $ExpectedText"
    }
    Write-Host "[OK] Screenshot capture dry-run accepted synthetic evidence folder." -ForegroundColor Green
    $global:LASTEXITCODE = 0
}

try {
    New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null
    Set-Content -Path $reportPath -Value @(
        "# Synthetic FW MVP PIE Acceptance Report",
        "",
        "- Screenshot or capture folder: $OutputRoot"
    ) -Encoding UTF8

    $outsideRoot = Join-Path (Split-Path -Parent $OutputRoot) "Outside"
    New-Item -ItemType Directory -Path $outsideRoot -Force | Out-Null

    Invoke-CaptureDryRun -ExpectedText "[OK] FW MVP PIE screenshot capture dry-run passed."
    Invoke-CaptureDryRun -ExpectFailure -ExpectedText "Screenshot OutputPath must use a .png extension" -ExtraArgs @("-OutputPath", (Join-Path $OutputRoot "bad.jpg"))
    Invoke-CaptureDryRun -ExpectFailure -ExpectedText "Screenshot output must stay inside the evidence folder" -ExtraArgs @("-OutputPath", (Join-Path $outsideRoot "outside.png"))

    Write-Host "[OK] FW MVP PIE evidence capture self-test completed." -ForegroundColor Green
    $global:LASTEXITCODE = 0
}
finally {
    if ($autoOutputRoot -and -not $KeepOutput -and (Test-Path $OutputRoot)) {
        Remove-Item -LiteralPath $OutputRoot -Recurse -Force
    }
}
