param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [string]$ProjectFile = (Join-Path (Split-Path -Parent $PSScriptRoot) "FW.uproject"),
    [int]$TimeoutSeconds = 900
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$editorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if (-not (Test-Path $editorCmd)) {
    throw "UnrealEditor-Cmd.exe not found: $editorCmd"
}

if (-not (Test-Path $ProjectFile)) {
    throw "Project file not found: $ProjectFile"
}

$logPath = Join-Path $repoRoot "Saved\Logs\FW.log"
$previousLogWrite = if (Test-Path $logPath) { (Get-Item $logPath).LastWriteTimeUtc } else { [datetime]::MinValue }

$arguments = @(
    "`"$ProjectFile`"",
    "-run=FWMVPFullLoopVerify",
    "-unattended",
    "-nop4",
    "-nosplash",
    "-NullRHI",
    "-NoSound"
)

$process = Start-Process -FilePath $editorCmd -ArgumentList $arguments -PassThru -WindowStyle Hidden
if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
    Stop-Process -Id $process.Id -Force
    throw "MVP headless full-loop verification timed out after $TimeoutSeconds seconds. Check Saved\Logs\FW.log."
}

if ($process.ExitCode -ne 0) {
    throw "MVP headless full-loop verification failed with exit code $($process.ExitCode). Check Saved\Logs\FW.log."
}

if (-not (Test-Path $logPath)) {
    throw "MVP headless full-loop verification did not produce Saved\Logs\FW.log."
}

$log = Get-Item $logPath
if ($log.LastWriteTimeUtc -le $previousLogWrite) {
    throw "MVP headless full-loop verification did not update Saved\Logs\FW.log."
}

$logText = Get-Content -Raw $logPath
if ($logText -notmatch "FW MVP headless full-loop verification passed\.") {
    throw "MVP headless full-loop verification did not emit the success marker. Check Saved\Logs\FW.log."
}

Write-Host "FW MVP headless full-loop verification completed."
