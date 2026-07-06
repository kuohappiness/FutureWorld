param(
    [string]$EvidenceFolder,
    [string]$ReportPath = (Join-Path (Split-Path -Parent $PSScriptRoot) "docs\FW_MVP_PIE_Acceptance_Report.md"),
    [string]$OutputPrefix = ("FW_MVP_PIE_WeaponEvidence_{0}" -f (Get-Date -Format "yyyyMMdd-HHmmss")),
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
    Write-Host "[OK] FW MVP PIE weapon evidence dry-run passed." -ForegroundColor Green
    Write-Host "Evidence folder: $((Resolve-Path -LiteralPath $EvidenceFolder).Path)"
    Write-Host "Output prefix: $OutputPrefix"
    exit 0
}

$editorProcess = Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue | Sort-Object StartTime -Descending | Select-Object -First 1
if (-not $editorProcess) {
    throw "UnrealEditor is not running. Start PIE before weapon evidence capture."
}

Add-Type @'
using System;
using System.Runtime.InteropServices;
public class FWInputEvidenceWin32 {
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
}
'@

function Get-EditorWindowHandle {
    param([System.Diagnostics.Process]$Process)

    $targetPid = [uint32]$Process.Id
    $script:editorWindowCandidate = [IntPtr]::Zero
    $script:editorWindowScore = [int64]::MinValue
    [FWInputEvidenceWin32]::EnumWindows({
        param($hWnd, $lParam)

        if (-not [FWInputEvidenceWin32]::IsWindowVisible($hWnd)) {
            return $true
        }

        $windowPid = [uint32]0
        [void][FWInputEvidenceWin32]::GetWindowThreadProcessId($hWnd, [ref]$windowPid)
        if ($windowPid -ne $targetPid) {
            return $true
        }

        $rect = New-Object FWInputEvidenceWin32+RECT
        if (-not [FWInputEvidenceWin32]::GetWindowRect($hWnd, [ref]$rect)) {
            return $true
        }

        $width = $rect.Right - $rect.Left
        $height = $rect.Bottom - $rect.Top
        if ($width -lt 320 -or $height -lt 240) {
            return $true
        }

        $classSb = New-Object System.Text.StringBuilder 256
        $titleSb = New-Object System.Text.StringBuilder 512
        [void][FWInputEvidenceWin32]::GetClassName($hWnd, $classSb, $classSb.Capacity)
        [void][FWInputEvidenceWin32]::GetWindowText($hWnd, $titleSb, $titleSb.Capacity)
        $className = $classSb.ToString()
        $title = $titleSb.ToString()

        $score = [int64]($width * $height)
        if ($className -eq "UnrealWindow") {
            $score += 100000000
        }
        if ($title -like "*Unreal Editor*") {
            $score += 50000000
        }
        if ($className -eq "ConsoleWindowClass" -or $title -like "*.exe*") {
            $score -= 100000000
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

    [void][FWInputEvidenceWin32]::ShowWindowAsync($WindowHandle, 3)
    [void][FWInputEvidenceWin32]::SetWindowPos($WindowHandle, $hwndTopMost, 0, 0, 0, 0, $flags)
    [void][FWInputEvidenceWin32]::BringWindowToTop($WindowHandle)
    [void][FWInputEvidenceWin32]::SetForegroundWindow($WindowHandle)
    Start-Sleep -Milliseconds 250
    [void][FWInputEvidenceWin32]::SetWindowPos($WindowHandle, $hwndNoTopMost, 0, 0, 0, 0, $flags)
}

function Invoke-KeyTap {
    param([byte]$VirtualKey)

    [FWInputEvidenceWin32]::keybd_event($VirtualKey, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 90
    [FWInputEvidenceWin32]::keybd_event($VirtualKey, 0, 0x0002, [UIntPtr]::Zero)
}

function Invoke-MouseClick {
    param(
        [int]$X,
        [int]$Y
    )

    [void][FWInputEvidenceWin32]::SetCursorPos($X, $Y)
    Start-Sleep -Milliseconds 120
    [FWInputEvidenceWin32]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 80
    [FWInputEvidenceWin32]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
}

function Capture-Step {
    param([string]$Suffix)

    $outputName = "$OutputPrefix`_$Suffix.png"
    & $captureScript -EvidenceFolder $EvidenceFolder -OutputName $outputName -WindowOnly -ProcessName "UnrealEditor" -SkipPrintWindow
    if (-not $?) {
        throw "Screenshot capture failed for weapon step: $Suffix"
    }
}

$mainWindow = Get-EditorWindowHandle $editorProcess
if ($mainWindow -eq [IntPtr]::Zero) {
    throw "UnrealEditor capturable window is not available."
}

$rect = New-Object FWInputEvidenceWin32+RECT
if (-not [FWInputEvidenceWin32]::GetWindowRect($mainWindow, [ref]$rect)) {
    throw "Failed to read UnrealEditor window bounds."
}

$centerX = $rect.Left + [int](($rect.Right - $rect.Left) * 0.5)
$centerY = $rect.Top + [int](($rect.Bottom - $rect.Top) * 0.55)

Set-EditorWindowForeground -WindowHandle $mainWindow
Invoke-MouseClick -X $centerX -Y $centerY
Start-Sleep -Milliseconds $StepDelayMilliseconds
Capture-Step "00-initial"

Invoke-KeyTap 0x32
Start-Sleep -Milliseconds $StepDelayMilliseconds
Capture-Step "01-slot2-rifle"

Invoke-KeyTap 0x33
Start-Sleep -Milliseconds $StepDelayMilliseconds
Capture-Step "02-slot3-sniper"

Invoke-KeyTap 0x31
Start-Sleep -Milliseconds $StepDelayMilliseconds
Capture-Step "03-slot1-pistol"

Invoke-MouseClick -X $centerX -Y $centerY
Start-Sleep -Milliseconds $StepDelayMilliseconds
Capture-Step "04-fire-pistol"

Write-Host "[OK] FW MVP PIE weapon evidence capture completed." -ForegroundColor Green
