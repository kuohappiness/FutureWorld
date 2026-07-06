param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [string]$ProjectFile = (Join-Path (Split-Path -Parent $PSScriptRoot) "FW.uproject"),
    [int]$TimeoutSeconds = 600
)

$ErrorActionPreference = "Stop"

$editorCmd = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if (-not (Test-Path $editorCmd)) {
    throw "UnrealEditor-Cmd.exe not found: $editorCmd"
}

if (-not (Test-Path $ProjectFile)) {
    throw "Project file not found: $ProjectFile"
}

$arguments = @(
    "`"$ProjectFile`"",
    "-run=FWMVPAssetBootstrap",
    "-unattended",
    "-nop4",
    "-nosplash",
    "-NullRHI",
    "-NoSound"
)

$process = Start-Process -FilePath $editorCmd -ArgumentList $arguments -PassThru -WindowStyle Hidden
if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
    Stop-Process -Id $process.Id -Force
    throw "Asset bootstrap timed out after $TimeoutSeconds seconds. Check Saved\Logs\FW.log."
}

if ($process.ExitCode -ne 0) {
    throw "Asset bootstrap failed with exit code $($process.ExitCode). Check Saved\Logs\FW.log."
}

Write-Host "FW MVP asset bootstrap completed."
