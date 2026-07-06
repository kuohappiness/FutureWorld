param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [string]$ProjectFile = (Join-Path (Split-Path -Parent $PSScriptRoot) "FW.uproject"),
    [string]$Tester = $env:USERNAME,
    [int]$TimeoutSeconds = 900,
    [int]$SmokeTimeoutSeconds = 900,
    [string]$EvidenceRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) "Saved\PIEAcceptance"),
    [string]$SessionName = (Get-Date -Format "yyyyMMdd-HHmmss"),
    [switch]$ListPreflightGates,
    [switch]$SkipPreflight
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$sessionDir = Join-Path $EvidenceRoot $SessionName
$logDir = Join-Path $sessionDir "Logs"
$reportPath = Join-Path $repoRoot "docs\FW_MVP_PIE_Acceptance_Report.md"

$preflightSteps = @(
    [ordered]@{
        Name = "Build"
        Command = {
            powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "Test-UnrealToolchain.ps1") -Build -EngineRoot $EngineRoot -ProjectFile $ProjectFile
        }
    },
    [ordered]@{
        Name = "AssetBootstrap"
        Command = {
            powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "Run-FWMVPAssetBootstrap.ps1") -EngineRoot $EngineRoot -ProjectFile $ProjectFile -TimeoutSeconds $TimeoutSeconds
        }
    },
    [ordered]@{
        Name = "MVPVerify"
        Command = {
            powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "Run-FWMVPVerify.ps1") -EngineRoot $EngineRoot -ProjectFile $ProjectFile -TimeoutSeconds $TimeoutSeconds
        }
    },
    [ordered]@{
        Name = "RuntimeVerify"
        Command = {
            powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "Run-FWMVPRuntimeVerify.ps1") -EngineRoot $EngineRoot -ProjectFile $ProjectFile -TimeoutSeconds $TimeoutSeconds
        }
    },
    [ordered]@{
        Name = "FullLoopVerify"
        Command = {
            powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "Run-FWMVPFullLoopVerify.ps1") -EngineRoot $EngineRoot -ProjectFile $ProjectFile -TimeoutSeconds $TimeoutSeconds
        }
    },
    [ordered]@{
        Name = "SmokeTest"
        Command = {
            powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "Run-FWMVPSmokeTest.ps1") -EngineRoot $EngineRoot -ProjectFile $ProjectFile -TimeoutSeconds $SmokeTimeoutSeconds
        }
    },
    [ordered]@{
        Name = "Readiness"
        Command = {
            powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "Test-FWMVPReadiness.ps1") -StrictAssets -VerifyPIEValidator
        }
    },
    [ordered]@{
        Name = "PIELaunchDryRun"
        Command = {
            powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "Open-FWMVPCombatGaragePIE.ps1") -EngineRoot $EngineRoot -ProjectFile $ProjectFile -DryRun
        }
    }
)

if ($ListPreflightGates) {
    foreach ($step in $preflightSteps) {
        Write-Host "[$($step.Name)]"
    }
    exit 0
}

New-Item -ItemType Directory -Path $logDir -Force | Out-Null

$manifest = [ordered]@{
    CreatedAt = (Get-Date).ToString("o")
    Tester = $Tester
    ProjectFile = $ProjectFile
    SessionDir = $sessionDir
    SkipPreflight = [bool]$SkipPreflight
    Gates = @()
}

function Write-Manifest {
    $script:manifest | ConvertTo-Json -Depth 6 | Set-Content -Path (Join-Path $sessionDir "manifest.json") -Encoding UTF8
}

function Write-SessionReadme {
    $sessionReadme = @(
        "# FW MVP PIE Acceptance Session",
        "",
        "- Created: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')",
        "- Tester: $Tester",
        "- Project: $ProjectFile",
        "- Evidence folder: $sessionDir",
        "",
        "## Manual PIE Evidence To Add",
        "",
        "- Screenshots or video captures proving player-facing camera/framing/HUD/debug presentation.",
        "- Notes for any non-blocking issues found during PIE.",
        "- Links to issues for any blocking problems.",
        "",
        "Open the correct manual PIE map with:",
        "",
        '```',
        "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Open-FWMVPCombatGaragePIE.ps1 -ProjectFile `"$ProjectFile`" -StartPIE",
        "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Invoke-FWMVPPIEPlayEvidenceCapture.ps1 -EvidenceFolder `"$sessionDir`" -SkipInput",
        "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Export-FWMVPPIERuntimeEvidence.ps1 -EvidenceFolder `"$sessionDir`"",
        "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Capture-FWMVPPIEEvidenceScreenshot.ps1 -EvidenceFolder `"$sessionDir`"",
        '```',
        "",
        "Check the current acceptance state with:",
        "",
        '```',
        "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Get-FWMVPPIEAcceptanceStatus.ps1",
        '```',
        "",
        "After manual PIE is truly complete and screenshots or video are in this folder, finalize and validate with:",
        "",
        '```',
        "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Complete-FWMVPPIEAcceptanceReport.ps1 -EvidenceFolder `"$sessionDir`" -Tester `"$Tester`" -ConfirmManualPIEComplete -MarkAllPIEItemsPassed",
        "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Test-FWMVPPIEAcceptance.ps1",
        "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Test-FWMVPReadiness.ps1 -StrictAssets -VerifyPIEValidator -RequirePIEAcceptance",
        '```'
    ) -join [Environment]::NewLine

    Set-Content -Path (Join-Path $sessionDir "README.md") -Value $sessionReadme -Encoding UTF8
}

function Invoke-Gate {
    param(
        [string]$Name,
        [scriptblock]$Command
    )

    Write-Host "[RUN] $Name" -ForegroundColor Cyan
    $startedAt = Get-Date
    $gate = [ordered]@{
        Name = $Name
        StartedAt = $startedAt.ToString("o")
        FinishedAt = $null
        ExitCode = $null
        Status = "Running"
        LogFile = $null
        OutputLog = $null
    }
    $outputLog = Join-Path $logDir "$Name-Output.log"
    $gate.OutputLog = $outputLog
    Set-Content -Path $outputLog -Value "[$Name] Started at $($startedAt.ToString("o"))" -Encoding UTF8

    $fwLog = Join-Path $repoRoot "Saved\Logs\FW.log"
    $previousLogWrite = $null
    if (Test-Path $fwLog) {
        $previousLogWrite = (Get-Item -LiteralPath $fwLog).LastWriteTimeUtc
    }

    $exitCode = 0
    $caughtError = $null
    $global:LASTEXITCODE = 0
    try {
        & $Command *>&1 | Tee-Object -FilePath $outputLog -Append
        $exitCode = if ($null -ne $LASTEXITCODE) { $LASTEXITCODE } else { 0 }
    }
    catch {
        $caughtError = $_
        $exitCode = 1
        "[$Name] Exception: $($caughtError.Exception.Message)" | Tee-Object -FilePath $outputLog -Append
    }

    if (Test-Path $fwLog) {
        $currentLogWrite = (Get-Item -LiteralPath $fwLog).LastWriteTimeUtc
        if ($null -eq $previousLogWrite -or $currentLogWrite -gt $previousLogWrite) {
            $archivedLog = Join-Path $logDir "$Name-FW.log"
            Copy-Item -LiteralPath $fwLog -Destination $archivedLog -Force
            $gate.LogFile = $archivedLog
        }
    }

    $gate.FinishedAt = (Get-Date).ToString("o")
    $gate.ExitCode = $exitCode
    $gate.Status = if ($exitCode -eq 0) { "Passed" } else { "Failed" }
    "[$Name] Finished at $($gate.FinishedAt) with status $($gate.Status) and exit code $exitCode" | Tee-Object -FilePath $outputLog -Append
    $script:manifest.Gates += $gate
    Write-Manifest

    if ($caughtError) {
        throw $caughtError
    }
    if ($exitCode -ne 0) {
        throw "$Name failed with exit code $exitCode."
    }
}

Write-SessionReadme
Write-Manifest

if (-not $SkipPreflight) {
    foreach ($step in $preflightSteps) {
        Invoke-Gate $step.Name $step.Command
    }
}
Write-Manifest

if (Test-Path $reportPath) {
    $reportText = Get-Content -Raw $reportPath
    $reportText = $reportText -replace "(?m)^- Date: .*$", "- Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
    $reportText = $reportText -replace "(?m)^- Tester: .*$", "- Tester: $Tester"
    $reportText = $reportText -replace "(?m)^- Screenshot or capture folder: .*$", "- Screenshot or capture folder: $sessionDir"
    Set-Content -Path $reportPath -Value $reportText -Encoding UTF8
}

Write-Host "[OK] FW MVP PIE acceptance session created: $sessionDir" -ForegroundColor Green
