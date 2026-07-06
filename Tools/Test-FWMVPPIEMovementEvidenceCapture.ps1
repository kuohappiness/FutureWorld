param(
    [string]$OutputRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) "Saved\PIEMovementEvidenceCaptureSynthetic\Run-$PID"),
    [switch]$KeepOutput
)

$ErrorActionPreference = "Stop"

$scriptPath = Join-Path $PSScriptRoot "Invoke-FWMVPPIEMovementEvidenceCapture.ps1"
$autoOutputRoot = $PSBoundParameters.ContainsKey("OutputRoot") -eq $false

function Invoke-DryRun {
    param(
        [switch]$ExpectFailure,
        [string]$ExpectedText,
        [string]$EvidenceFolder = $OutputRoot
    )

    $args = @(
        "-NoProfile",
        "-ExecutionPolicy", "Bypass",
        "-File", $scriptPath,
        "-EvidenceFolder", $EvidenceFolder,
        "-DryRun"
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
            throw "PIE movement evidence dry-run was expected to fail but passed."
        }
        if ($ExpectedText -and $outputText -notmatch [regex]::Escape($ExpectedText)) {
            Write-Host $outputText
            throw "PIE movement evidence dry-run failure did not include expected text: $ExpectedText"
        }
        $global:LASTEXITCODE = 0
        Write-Host "[OK] PIE movement evidence dry-run rejected invalid synthetic input." -ForegroundColor Green
        return
    }

    if ($exitCode -ne 0) {
        Write-Host $outputText
        throw "PIE movement evidence dry-run was expected to pass but failed with exit code $exitCode."
    }
    if ($ExpectedText -and $outputText -notmatch [regex]::Escape($ExpectedText)) {
        Write-Host $outputText
        throw "PIE movement evidence dry-run output did not include expected text: $ExpectedText"
    }
    Write-Host "[OK] PIE movement evidence dry-run accepted synthetic evidence folder." -ForegroundColor Green
    $global:LASTEXITCODE = 0
}

try {
    New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null
    Invoke-DryRun -ExpectedText "[OK] FW MVP PIE movement evidence dry-run passed."
    Invoke-DryRun -ExpectFailure -ExpectedText "Evidence folder does not exist:" -EvidenceFolder (Join-Path $OutputRoot "Missing")

    Write-Host "[OK] FW MVP PIE movement evidence capture self-test completed." -ForegroundColor Green
    $global:LASTEXITCODE = 0
}
finally {
    if ($autoOutputRoot -and -not $KeepOutput -and (Test-Path $OutputRoot)) {
        Remove-Item -LiteralPath $OutputRoot -Recurse -Force
    }
}
