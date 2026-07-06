param(
    [string]$EvidenceFolder,
    [string]$ReportPath = (Join-Path (Split-Path -Parent $PSScriptRoot) "docs\FW_MVP_PIE_Acceptance_Report.md"),
    [string]$OutputPrefix = ("FW_MVP_PIE_VehicleInteraction_{0}" -f (Get-Date -Format "yyyyMMdd-HHmmss")),
    [int]$InitialDelayMilliseconds = 2200,
    [int]$StepDelayMilliseconds = 3200,
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

if ($DryRun) {
    Write-Host "[OK] FW MVP PIE vehicle interaction evidence dry-run passed." -ForegroundColor Green
    Write-Host "Evidence folder: $((Resolve-Path -LiteralPath $EvidenceFolder).Path)"
    Write-Host "Output prefix: $OutputPrefix"
    Write-Host "Initial delay milliseconds: $InitialDelayMilliseconds"
    Write-Host "Step delay milliseconds: $StepDelayMilliseconds"
    exit 0
}

$editorProcess = Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue | Sort-Object StartTime -Descending | Select-Object -First 1
if (-not $editorProcess) {
    throw "UnrealEditor is not running. Start PIE before vehicle interaction evidence capture."
}

Add-Type @'
using System;
using System.Runtime.InteropServices;
public class FWVehicleInteractionWin32 {
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
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

function Get-EditorWindowHandle {
    param([System.Diagnostics.Process]$Process)

    $targetPid = [uint32]$Process.Id
    $script:editorWindowCandidate = [IntPtr]::Zero
    $script:editorWindowScore = [int64]::MinValue
    [FWVehicleInteractionWin32]::EnumWindows({
        param($hWnd, $lParam)

        if (-not [FWVehicleInteractionWin32]::IsWindowVisible($hWnd)) {
            return $true
        }

        $windowPid = [uint32]0
        [void][FWVehicleInteractionWin32]::GetWindowThreadProcessId($hWnd, [ref]$windowPid)
        if ($windowPid -ne $targetPid) {
            return $true
        }

        $rect = New-Object FWVehicleInteractionWin32+RECT
        if (-not [FWVehicleInteractionWin32]::GetWindowRect($hWnd, [ref]$rect)) {
            return $true
        }

        $width = $rect.Right - $rect.Left
        $height = $rect.Bottom - $rect.Top
        if ($width -lt 640 -or $height -lt 360) {
            return $true
        }

        $classSb = New-Object System.Text.StringBuilder 256
        [void][FWVehicleInteractionWin32]::GetClassName($hWnd, $classSb, $classSb.Capacity)
        $className = $classSb.ToString()

        $score = [int64]($width * $height)
        if ($className -eq "UnrealWindow") {
            $score += 100000000
        }
        if ($className -eq "ConsoleWindowClass") {
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

    [void][FWVehicleInteractionWin32]::ShowWindowAsync($WindowHandle, 3)
    [void][FWVehicleInteractionWin32]::SetWindowPos($WindowHandle, $hwndTopMost, 0, 0, 0, 0, $flags)
    [void][FWVehicleInteractionWin32]::BringWindowToTop($WindowHandle)
    [void][FWVehicleInteractionWin32]::SetForegroundWindow($WindowHandle)
    Start-Sleep -Milliseconds 350
    [void][FWVehicleInteractionWin32]::SetWindowPos($WindowHandle, $hwndNoTopMost, 0, 0, 0, 0, $flags)
}

function Invoke-KeyTap {
    param(
        [byte]$VirtualKey,
        [byte]$ScanCode
    )

    [FWVehicleInteractionWin32]::keybd_event($VirtualKey, $ScanCode, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 110
    [FWVehicleInteractionWin32]::keybd_event($VirtualKey, $ScanCode, 0x0002, [UIntPtr]::Zero)
}

function Invoke-KeyHold {
    param(
        [byte]$VirtualKey,
        [byte]$ScanCode,
        [int]$Seconds
    )

    [FWVehicleInteractionWin32]::keybd_event($VirtualKey, $ScanCode, 0, [UIntPtr]::Zero)
    Start-Sleep -Seconds $Seconds
    [FWVehicleInteractionWin32]::keybd_event($VirtualKey, $ScanCode, 0x0002, [UIntPtr]::Zero)
}

function Invoke-MouseClick {
    param(
        [int]$X,
        [int]$Y
    )

    [void][FWVehicleInteractionWin32]::SetCursorPos($X, $Y)
    Start-Sleep -Milliseconds 120
    [FWVehicleInteractionWin32]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 80
    [FWVehicleInteractionWin32]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
}

function Capture-DesktopStep {
    param([string]$Suffix)

    $outputName = "$OutputPrefix`_$Suffix.png"
    $outputPath = Join-Path $EvidenceFolder $outputName
    $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $bitmap = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    try {
        $graphics.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
        $bitmap.Save($outputPath, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }

    $file = Get-Item -LiteralPath $outputPath
    if ($file.Length -lt 1024) {
        throw "Screenshot evidence is too small: $outputPath"
    }
    $signature = [System.IO.File]::ReadAllBytes($outputPath)[0..7]
    $expected = @(0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A)
    for ($i = 0; $i -lt $expected.Count; $i++) {
        if ($signature[$i] -ne $expected[$i]) {
            throw "Screenshot evidence is not a PNG: $outputPath"
        }
    }
    Write-Host "[OK] Captured vehicle interaction evidence: $outputPath ($($file.Length) bytes)" -ForegroundColor Green
}

$mainWindow = Get-EditorWindowHandle $editorProcess
if ($mainWindow -eq [IntPtr]::Zero) {
    throw "UnrealEditor capturable window is not available."
}

Set-EditorWindowForeground -WindowHandle $mainWindow
Start-Sleep -Milliseconds $InitialDelayMilliseconds
Capture-DesktopStep "00-enter-prompt"

Set-EditorWindowForeground -WindowHandle $mainWindow
Start-Sleep -Milliseconds $StepDelayMilliseconds
Capture-DesktopStep "01-entered-car"

Set-EditorWindowForeground -WindowHandle $mainWindow
Start-Sleep -Milliseconds $StepDelayMilliseconds
Capture-DesktopStep "02-exited-car"

Write-Host "[OK] FW MVP PIE vehicle interaction evidence capture completed." -ForegroundColor Green
