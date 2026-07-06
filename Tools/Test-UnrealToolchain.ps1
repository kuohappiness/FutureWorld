param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [string]$ProjectFile = (Join-Path (Split-Path -Parent $PSScriptRoot) "FW.uproject"),
    [string]$AsciiProjectFile = "D:\Projects\FutureWorld\FW.uproject",
    [switch]$Build
)

$ErrorActionPreference = "Stop"

function Write-Check($Name, $Passed, $Detail) {
    $status = if ($Passed) { "OK" } else { "MISSING" }
    $color = if ($Passed) { "Green" } else { "Red" }
    Write-Host ("[{0}] {1}: {2}" -f $status, $Name, $Detail) -ForegroundColor $color
}

$editor = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"
$ubt = Join-Path $EngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
$dotnetRoot = Join-Path $EngineRoot "Engine\Binaries\ThirdParty\DotNet\10.0\win-x64"
$dotnet = Join-Path $dotnetRoot "dotnet.exe"
$vswhereCandidates = @(
    "C:\Program Files\Microsoft Visual Studio\Installer\vswhere.exe",
    "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
)
$vswhere = $vswhereCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
$windowsSdkLib = "C:\Program Files (x86)\Windows Kits\10\Lib"

$hasEditor = Test-Path $editor
$hasUbt = Test-Path $ubt
$hasDotnet = Test-Path $dotnet
$hasVswhere = $null -ne $vswhere
$sdkVersions = if (Test-Path $windowsSdkLib) {
    Get-ChildItem $windowsSdkLib -Directory -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Name
} else {
    @()
}
$hasWindowsSdk = $sdkVersions.Count -gt 0

Write-Check "UnrealEditor" $hasEditor $editor
Write-Check "UnrealBuildTool" $hasUbt $ubt
Write-Check "UE bundled dotnet" $hasDotnet $dotnet
Write-Check "Visual Studio Installer / vswhere" $hasVswhere $(if ($vswhere) { $vswhere } else { $vswhereCandidates -join ", " })
Write-Check "Windows SDK Lib" $hasWindowsSdk ($sdkVersions -join ", ")

if ($hasVswhere) {
    $vs = & $vswhere -all -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json | ConvertFrom-Json
    Write-Check "MSVC C++ tools" ($vs.Count -gt 0) (($vs | ForEach-Object { $_.installationPath }) -join ", ")
}

$ready = $hasEditor -and $hasUbt -and $hasDotnet -and $hasVswhere -and $hasWindowsSdk
if (-not $ready) {
    Write-Host ""
    Write-Host "Install Visual Studio Build Tools with:" -ForegroundColor Yellow
    Write-Host "  - Desktop development with C++"
    Write-Host "  - MSVC v143 or newer x64/x86 build tools"
    Write-Host "  - Windows 10 SDK 10.0.19041.0 or newer"
    Write-Host ""
    exit 2
}

if ($Build) {
    $buildProjectFile = $ProjectFile
    if ((Test-Path $AsciiProjectFile) -and ($ProjectFile -match "[^\x00-\x7F]")) {
        Write-Host "Using ASCII project path for UnrealBuildTool: $AsciiProjectFile" -ForegroundColor Yellow
        $buildProjectFile = $AsciiProjectFile
    }

    $env:DOTNET_ROOT = $dotnetRoot
    $env:PATH = "$dotnetRoot;$env:PATH"
    & $ubt FWEditor Win64 Development -Project="$buildProjectFile" -WaitMutex -NoUBA -NoLiveCoding
}
