param(
    [string]$EvidenceFolder,
    [string]$ReportPath = (Join-Path (Split-Path -Parent $PSScriptRoot) "docs\FW_MVP_PIE_Acceptance_Report.md"),
    [string]$OutputPrefix = ("FW_MVP_PIE_MovementEvidence_{0}" -f (Get-Date -Format "yyyyMMdd-HHmmss")),
    [int]$StepDelayMilliseconds = 1200,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

if (-not $EvidenceFolder) {
    if (-not (Test-Path $ReportPath -PathType Leaf)) {
        throw "PIE acceptance report not found: $ReportPath"
    }
    $reportText = Get-Content -Raw $ReportPath
    if ($reportText -match "Screenshot or capture folder:\s*(.+)") {
        $EvidenceFolder = $Matches[1].Trim()
    }
}
if (-not $EvidenceFolder -or $EvidenceFolder -eq "TBD") {
    throw "EvidenceFolder is required, or the report must already contain a screenshot/capture folder."
}
if (-not (Test-Path $EvidenceFolder -PathType Container)) {
    throw "Evidence folder does not exist: $EvidenceFolder"
}

$captureScript = Join-Path $PSScriptRoot "Capture-FWMVPPIEEvidenceScreenshot.ps1"
if (-not (Test-Path $captureScript -PathType Leaf)) {
    throw "Screenshot capture script not found: $captureScript"
}

if ($DryRun) {
    Write-Host "[OK] FW MVP PIE movement evidence dry-run passed." -ForegroundColor Green
    Write-Host "Evidence folder: $((Resolve-Path -LiteralPath $EvidenceFolder).Path)"
    Write-Host "Output prefix: $OutputPrefix"
    Write-Host "Step delay milliseconds: $StepDelayMilliseconds"
    exit 0
}

$editorProcess = Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue | Sort-Object StartTime -Descending | Select-Object -First 1
if (-not $editorProcess) {
    throw "UnrealEditor is not running. Start PIE before movement evidence capture."
}

Add-Type @'
using System;
using System.Runtime.InteropServices;
public class FWMovementEvidenceWin32 {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr hWnd, System.Text.StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern int GetClassName(IntPtr hWnd, System.Text.StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool ShowWindowAsync(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int X, int Y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint dwData, UIntPtr dwExtraInfo);
    [DllImport("user32.dll")] public static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, UIntPtr dwExtraInfo);
    [DllImport("user32.dll")] public static extern uint MapVirtualKey(uint uCode, uint uMapType);
}
'@

function Get-EditorWindowHandle {
    param([System.Diagnostics.Process]$Process)

    $targetPid = [uint32]$Process.Id
    $script:editorWindowCandidate = [IntPtr]::Zero
    $script:editorWindowScore = [int64]::MinValue
    [FWMovementEvidenceWin32]::EnumWindows({
        param($hWnd, $lParam)

        $windowPid = [uint32]0
        [void][FWMovementEvidenceWin32]::GetWindowThreadProcessId($hWnd, [ref]$windowPid)
        if ($windowPid -ne $targetPid) {
            return $true
        }

        $rect = New-Object FWMovementEvidenceWin32+RECT
        if (-not [FWMovementEvidenceWin32]::GetWindowRect($hWnd, [ref]$rect)) {
            return $true
        }

        $width = $rect.Right - $rect.Left
        $height = $rect.Bottom - $rect.Top
        if ($width -lt 320 -or $height -lt 240) {
            return $true
        }

        $classSb = New-Object System.Text.StringBuilder 256
        $titleSb = New-Object System.Text.StringBuilder 512
        [void][FWMovementEvidenceWin32]::GetClassName($hWnd, $classSb, $classSb.Capacity)
        [void][FWMovementEvidenceWin32]::GetWindowText($hWnd, $titleSb, $titleSb.Capacity)
        $className = $classSb.ToString()
        $title = $titleSb.ToString()
        if ($className -eq "ConsoleWindowClass" -or $title -like "*.exe*") {
            return $true
        }
        if (-not [FWMovementEvidenceWin32]::IsWindowVisible($hWnd) -and $className -ne "UnrealWindow") {
            return $true
        }

        $score = [int64]($width * $height)
        if ($className -eq "UnrealWindow") {
            $score += 100000000
        }
        if ($title -like "*Unreal Editor*") {
            $score += 50000000
        }
        if ($score -gt $script:editorWindowScore) {
            $script:editorWindowScore = $score
            $script:editorWindowCandidate = $hWnd
        }
        return $true
    }, [IntPtr]::Zero) | Out-Null

    return $script:editorWindowCandidate
}

function Set-EditorWindowForeground {
    param([IntPtr]$WindowHandle)

    $hwndTopMost = [IntPtr](-1)
    $hwndNoTopMost = [IntPtr](-2)
    $flags = 0x0002 -bor 0x0001 -bor 0x0040

    [void][FWMovementEvidenceWin32]::ShowWindowAsync($WindowHandle, 3)
    [void][FWMovementEvidenceWin32]::SetWindowPos($WindowHandle, $hwndTopMost, 0, 0, 0, 0, $flags)
    [void][FWMovementEvidenceWin32]::BringWindowToTop($WindowHandle)
    [void][FWMovementEvidenceWin32]::SetForegroundWindow($WindowHandle)
    Start-Sleep -Milliseconds 250
    [void][FWMovementEvidenceWin32]::SetWindowPos($WindowHandle, $hwndNoTopMost, 0, 0, 0, 0, $flags)
}

function Invoke-KeyDown {
    param([byte]$VirtualKey)
    $scanCode = [byte]([FWMovementEvidenceWin32]::MapVirtualKey($VirtualKey, 0) -band 0xff)
    [FWMovementEvidenceWin32]::keybd_event($VirtualKey, $scanCode, 0, [UIntPtr]::Zero)
}

function Invoke-KeyUp {
    param([byte]$VirtualKey)
    $scanCode = [byte]([FWMovementEvidenceWin32]::MapVirtualKey($VirtualKey, 0) -band 0xff)
    [FWMovementEvidenceWin32]::keybd_event($VirtualKey, $scanCode, 0x0002, [UIntPtr]::Zero)
}

function Invoke-KeyTap {
    param(
        [byte]$VirtualKey,
        [int]$HoldMilliseconds = 120
    )

    Invoke-KeyDown $VirtualKey
    Start-Sleep -Milliseconds $HoldMilliseconds
    Invoke-KeyUp $VirtualKey
}

function Invoke-MouseClick {
    param(
        [int]$X,
        [int]$Y
    )

    [void][FWMovementEvidenceWin32]::SetCursorPos($X, $Y)
    Start-Sleep -Milliseconds 120
    [FWMovementEvidenceWin32]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 80
    [FWMovementEvidenceWin32]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
}

function Capture-Step {
    param([string]$Suffix)

    $outputName = "$OutputPrefix`_$Suffix.png"
    & $captureScript -EvidenceFolder $EvidenceFolder -OutputName $outputName -WindowOnly -ProcessName "UnrealEditor" -SkipPrintWindow
    if (-not $?) {
        throw "Screenshot capture failed for movement step: $Suffix"
    }
}

function Invoke-HoldAndCapture {
    param(
        [string]$Suffix,
        [byte[]]$VirtualKeys,
        [int]$HoldMilliseconds
    )

    foreach ($virtualKey in $VirtualKeys) {
        Invoke-KeyDown $virtualKey
        Start-Sleep -Milliseconds 60
    }
    Start-Sleep -Milliseconds $HoldMilliseconds
    Capture-Step $Suffix
    [array]::Reverse($VirtualKeys)
    foreach ($virtualKey in $VirtualKeys) {
        Invoke-KeyUp $virtualKey
        Start-Sleep -Milliseconds 60
    }
    Start-Sleep -Milliseconds $StepDelayMilliseconds
}

$mainWindow = Get-EditorWindowHandle $editorProcess
if ($mainWindow -eq [IntPtr]::Zero) {
    throw "UnrealEditor capturable window is not available."
}

$rect = New-Object FWMovementEvidenceWin32+RECT
if (-not [FWMovementEvidenceWin32]::GetWindowRect($mainWindow, [ref]$rect)) {
    throw "Failed to read UnrealEditor window bounds."
}

$centerX = $rect.Left + [int](($rect.Right - $rect.Left) * 0.5)
$centerY = $rect.Top + [int](($rect.Bottom - $rect.Top) * 0.55)

Set-EditorWindowForeground -WindowHandle $mainWindow
Invoke-MouseClick -X $centerX -Y $centerY
Start-Sleep -Milliseconds $StepDelayMilliseconds
Capture-Step "00-initial"

Invoke-HoldAndCapture -Suffix "01-walk-forward" -VirtualKeys ([byte[]](0x57)) -HoldMilliseconds 1400
Invoke-HoldAndCapture -Suffix "02-run-forward" -VirtualKeys ([byte[]](0xA0, 0x57)) -HoldMilliseconds 1400

Invoke-KeyTap -VirtualKey 0x20 -HoldMilliseconds 160
Start-Sleep -Milliseconds 450
Capture-Step "03-jump"
Start-Sleep -Milliseconds $StepDelayMilliseconds

Invoke-KeyTap -VirtualKey 0x11 -HoldMilliseconds 160
Start-Sleep -Milliseconds $StepDelayMilliseconds
Capture-Step "04-crouch"

Invoke-KeyTap -VirtualKey 0x43 -HoldMilliseconds 160
Start-Sleep -Milliseconds $StepDelayMilliseconds
Capture-Step "05-crawl"

Invoke-KeyTap -VirtualKey 0x43 -HoldMilliseconds 160
Start-Sleep -Milliseconds 300
Invoke-KeyTap -VirtualKey 0x11 -HoldMilliseconds 160
Start-Sleep -Milliseconds $StepDelayMilliseconds
Capture-Step "06-restored"

Write-Host "[OK] FW MVP PIE movement evidence capture completed." -ForegroundColor Green
