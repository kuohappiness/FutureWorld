param(
    [switch]$UpdateProject,
    [string]$EngineRoot,
    [string]$ProjectFile = (Join-Path (Split-Path -Parent $PSScriptRoot) "FW.uproject")
)

$ErrorActionPreference = "Stop"

function Get-RegistryEngineInstallations {
    $installations = @()

    $machineKey = "HKLM:\SOFTWARE\EpicGames\Unreal Engine"
    if (Test-Path $machineKey) {
        Get-ChildItem $machineKey -ErrorAction SilentlyContinue | ForEach-Object {
            $props = Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue
            if ($props.InstalledDirectory) {
                $installations += [pscustomobject]@{
                    Version = $_.PSChildName
                    Root = $props.InstalledDirectory
                    Source = "HKLM EpicGames"
                }
            }
        }
    }

    $userBuildsKey = "HKCU:\Software\Epic Games\Unreal Engine\Builds"
    if (Test-Path $userBuildsKey) {
        $props = Get-ItemProperty $userBuildsKey -ErrorAction SilentlyContinue
        $props.PSObject.Properties |
            Where-Object { $_.Name -notlike "PS*" -and $_.Value } |
            ForEach-Object {
                $installations += [pscustomobject]@{
                    Version = $_.Name
                    Root = $_.Value
                    Source = "HKCU Builds"
                }
            }
    }

    return $installations
}

function Get-CommonEngineInstallations {
    $workspaceRoot = Split-Path -Parent $PSScriptRoot
    $workspaceParent = Split-Path -Parent $workspaceRoot

    $roots = @(
        "C:\Program Files\Epic Games",
        "C:\Program Files (x86)\Epic Games",
        "D:\Epic Games",
        "D:\Unreal",
        "D:\UnrealEngine",
        $workspaceParent
    ) | Where-Object { $_ } | Sort-Object -Unique

    foreach ($root in $roots) {
        if (-not (Test-Path $root)) {
            continue
        }

        Get-ChildItem $root -Directory -ErrorAction SilentlyContinue |
            Where-Object { $_.Name -match "^(UE_)?5(\.|_)" -or $_.Name -match "^UnrealEngine" } |
            ForEach-Object {
                [pscustomobject]@{
                    Version = $_.Name
                    Root = $_.FullName
                    Source = "Common path"
                }
            }
    }
}

function Test-EngineInstallation($installation) {
    $editor = Join-Path $installation.Root "Engine\Binaries\Win64\UnrealEditor.exe"
    $buildTool = Join-Path $installation.Root "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"

    if (-not (Test-Path $buildTool)) {
        $legacyBuildTool = Join-Path $installation.Root "Engine\Binaries\DotNET\UnrealBuildTool.exe"
        if (Test-Path $legacyBuildTool) {
            $buildTool = $legacyBuildTool
        }
    }

    [pscustomobject]@{
        Version = $installation.Version
        Root = $installation.Root
        Source = $installation.Source
        UnrealEditor = if (Test-Path $editor) { $editor } else { $null }
        UnrealBuildTool = if (Test-Path $buildTool) { $buildTool } else { $null }
        IsUsable = (Test-Path $editor) -and (Test-Path $buildTool)
    }
}

$candidates = @()
$candidates += Get-RegistryEngineInstallations
$candidates += Get-CommonEngineInstallations

if ($EngineRoot) {
    $candidates += [pscustomobject]@{
        Version = Split-Path -Leaf $EngineRoot
        Root = $EngineRoot
        Source = "Explicit EngineRoot"
    }
}

$engines = $candidates |
    Sort-Object Root -Unique |
    ForEach-Object { Test-EngineInstallation $_ } |
    Sort-Object Version -Descending

if (-not $engines) {
    Write-Host "No Unreal Engine installation was found in registry or common paths." -ForegroundColor Yellow
    Write-Host "Install UE5 with Epic Games Launcher, then rerun this script:" -ForegroundColor Yellow
    Write-Host "  powershell -ExecutionPolicy Bypass -File Tools\Find-UnrealEngine.ps1 -UpdateProject"
    exit 2
}

$engines | Format-Table Version, IsUsable, Root, Source -AutoSize

$usable = $engines | Where-Object { $_.IsUsable } | Select-Object -First 1
if (-not $usable) {
    Write-Host "Unreal Engine folders were found, but required executables are missing." -ForegroundColor Red
    $engines | Format-List Version, Root, UnrealEditor, UnrealBuildTool
    exit 3
}

Write-Host "Selected Unreal Engine:" -ForegroundColor Green
$usable | Format-List Version, Root, UnrealEditor, UnrealBuildTool

if ($UpdateProject) {
    if (-not (Test-Path $ProjectFile)) {
        throw "Project file not found: $ProjectFile"
    }

    $json = Get-Content -Raw -Path $ProjectFile | ConvertFrom-Json
    $json | Add-Member -NotePropertyName EngineAssociation -NotePropertyValue $usable.Version -Force
    $json | ConvertTo-Json -Depth 10 | Set-Content -Path $ProjectFile -Encoding UTF8
    Write-Host "Updated EngineAssociation in $ProjectFile to '$($usable.Version)'." -ForegroundColor Green
}
