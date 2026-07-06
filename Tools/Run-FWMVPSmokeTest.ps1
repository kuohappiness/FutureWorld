param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [string]$ProjectFile = (Join-Path (Split-Path -Parent $PSScriptRoot) "FW.uproject"),
    [string]$TestName = "FW.MVP.Smoke.CoreDataAndState",
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
    "-ExecCmds=`"Automation RunTests $TestName;Quit`"",
    "-unattended",
    "-nop4",
    "-nosplash",
    "-NullRHI",
    "-NoSound"
)

$process = Start-Process -FilePath $editorCmd -ArgumentList $arguments -PassThru -WindowStyle Hidden
if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
    Stop-Process -Id $process.Id -Force
    throw "Smoke test timed out after $TimeoutSeconds seconds. Check Saved\Logs\FW.log for Automation output."
}

if ($process.ExitCode -ne 0) {
    throw "Smoke test failed with exit code $($process.ExitCode). Check Saved\Logs\FW.log."
}

$projectRoot = Split-Path -Parent $ProjectFile
$logPath = Join-Path $projectRoot "Saved\Logs\FW.log"
if (-not (Test-Path $logPath)) {
    throw "Smoke test log not found: $logPath"
}

$logText = Get-Content -Path $logPath -Raw
$escapedTestName = [regex]::Escape($TestName)
if ($logText -notmatch "Found 1 automation tests based on '$escapedTestName'") {
    throw "Smoke test did not discover exactly one automation test named $TestName. Check Saved\Logs\FW.log."
}

if ($logText -notmatch "Test Completed\. Result=\{Success\} Name=\{[^}]+\} Path=\{$escapedTestName\}") {
    throw "Smoke test did not report success for $TestName. Check Saved\Logs\FW.log."
}

if ($logText -notmatch "\*\*\*\* TEST COMPLETE\. EXIT CODE: 0 \*\*\*\*") {
    throw "Smoke test automation runner did not report exit code 0. Check Saved\Logs\FW.log."
}

Write-Host "Smoke test completed: $TestName"
