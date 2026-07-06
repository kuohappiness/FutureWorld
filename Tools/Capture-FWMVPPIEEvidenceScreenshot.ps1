param(
    [string]$ReportPath = (Join-Path (Split-Path -Parent $PSScriptRoot) "docs\FW_MVP_PIE_Acceptance_Report.md"),
    [string]$EvidenceFolder,
    [string]$OutputName = ("FW_MVP_PIE_Evidence_{0}.png" -f (Get-Date -Format "yyyyMMdd-HHmmss")),
    [string]$OutputPath,
    [switch]$WindowOnly,
    [string]$ProcessName = "UnrealEditor",
    [string]$WindowClassName = "UnrealWindow",
    [string]$WindowTitlePattern = "*Unreal Editor*",
    [switch]$SkipPrintWindow,
    [switch]$ListWindowCandidates,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

if (-not ("Win32CaptureBounds" -as [type])) {
Add-Type @'
using System;
using System.Runtime.InteropServices;
public class Win32CaptureBounds {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr hWnd, System.Text.StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern int GetClassName(IntPtr hWnd, System.Text.StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool ShowWindowAsync(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr hWnd, IntPtr hdcBlt, uint nFlags);
}
'@
}

function Set-CaptureWindowForeground {
    param([IntPtr]$WindowHandle)

    $hwndTopMost = [IntPtr](-1)
    $hwndNoTopMost = [IntPtr](-2)
    $swpNoMove = 0x0002
    $swpNoSize = 0x0001
    $swpShowWindow = 0x0040
    $flags = $swpNoMove -bor $swpNoSize -bor $swpShowWindow

    [void][Win32CaptureBounds]::ShowWindowAsync($WindowHandle, 3)
    [void][Win32CaptureBounds]::SetWindowPos($WindowHandle, $hwndTopMost, 0, 0, 0, 0, $flags)
    [void][Win32CaptureBounds]::BringWindowToTop($WindowHandle)
    [void][Win32CaptureBounds]::SetForegroundWindow($WindowHandle)
    Start-Sleep -Milliseconds 250
    [void][Win32CaptureBounds]::SetWindowPos($WindowHandle, $hwndNoTopMost, 0, 0, 0, 0, $flags)
}

function Get-CaptureWindowHandle {
    param([System.Diagnostics.Process]$Process)

    $targetPid = if ($Process) { [uint32]$Process.Id } else { [uint32]0 }
    $script:captureCandidate = [IntPtr]::Zero
    $script:captureCandidateScore = [int64]::MinValue
    $script:targetWindowClassName = $WindowClassName
    $script:targetWindowTitlePattern = $WindowTitlePattern
    [Win32CaptureBounds]::EnumWindows({
        param($hWnd, $lParam)

        $windowPid = [uint32]0
        [void][Win32CaptureBounds]::GetWindowThreadProcessId($hWnd, [ref]$windowPid)
        if ($targetPid -ne 0 -and $windowPid -ne $targetPid) {
            return $true
        }

        $classSb = New-Object System.Text.StringBuilder 256
        $titleSb = New-Object System.Text.StringBuilder 512
        [void][Win32CaptureBounds]::GetClassName($hWnd, $classSb, $classSb.Capacity)
        [void][Win32CaptureBounds]::GetWindowText($hWnd, $titleSb, $titleSb.Capacity)
        $className = $classSb.ToString()
        $title = $titleSb.ToString()
        if ($className -eq "ConsoleWindowClass" -or $title -like "*.exe*") {
            return $true
        }
        if (-not [Win32CaptureBounds]::IsWindowVisible($hWnd) -and $className -ne $script:targetWindowClassName) {
            return $true
        }

        $rect = New-Object Win32CaptureBounds+RECT
        if ([Win32CaptureBounds]::GetWindowRect($hWnd, [ref]$rect)) {
            $width = $rect.Right - $rect.Left
            $height = $rect.Bottom - $rect.Top
            if ($width -ge 320 -and $height -ge 240) {
                $score = [int64]($width * $height)
                if ($className -eq $script:targetWindowClassName) {
                    $score += 100000000
                }
                if ($title -like $script:targetWindowTitlePattern) {
                    $score += 50000000
                }
                if ($score -gt $script:captureCandidateScore) {
                    $script:captureCandidateScore = $score
                    $script:captureCandidate = $hWnd
                }
            }
        }
        return $true
    }, [IntPtr]::Zero) | Out-Null

    if ($script:captureCandidate -ne [IntPtr]::Zero) {
        return $script:captureCandidate
    }

    [Win32CaptureBounds]::EnumWindows({
        param($hWnd, $lParam)

        $classSb = New-Object System.Text.StringBuilder 256
        $titleSb = New-Object System.Text.StringBuilder 512
        [void][Win32CaptureBounds]::GetClassName($hWnd, $classSb, $classSb.Capacity)
        [void][Win32CaptureBounds]::GetWindowText($hWnd, $titleSb, $titleSb.Capacity)
        if (-not [Win32CaptureBounds]::IsWindowVisible($hWnd) -and $classSb.ToString() -ne $script:targetWindowClassName) {
            return $true
        }
        if ($classSb.ToString() -ne $script:targetWindowClassName -or $titleSb.ToString() -notlike $script:targetWindowTitlePattern) {
            return $true
        }

        $rect = New-Object Win32CaptureBounds+RECT
        if ([Win32CaptureBounds]::GetWindowRect($hWnd, [ref]$rect)) {
            $width = $rect.Right - $rect.Left
            $height = $rect.Bottom - $rect.Top
            if ($width -ge 320 -and $height -ge 240) {
                $script:captureCandidate = $hWnd
                return $false
            }
        }
        return $true
    }, [IntPtr]::Zero) | Out-Null

    return $script:captureCandidate
}

if ($ListWindowCandidates) {
    $script:targetWindowClassName = $WindowClassName
    $script:targetWindowTitlePattern = $WindowTitlePattern
    $rows = New-Object System.Collections.Generic.List[object]
    [Win32CaptureBounds]::EnumWindows({
        param($hWnd, $lParam)

        if (-not [Win32CaptureBounds]::IsWindowVisible($hWnd)) {
            return $true
        }
        $classSb = New-Object System.Text.StringBuilder 256
        $titleSb = New-Object System.Text.StringBuilder 512
        [void][Win32CaptureBounds]::GetClassName($hWnd, $classSb, $classSb.Capacity)
        [void][Win32CaptureBounds]::GetWindowText($hWnd, $titleSb, $titleSb.Capacity)
        if ($classSb.ToString() -ne $script:targetWindowClassName -and $titleSb.ToString() -notlike $script:targetWindowTitlePattern) {
            return $true
        }
        $rect = New-Object Win32CaptureBounds+RECT
        [void][Win32CaptureBounds]::GetWindowRect($hWnd, [ref]$rect)
        $windowPid = [uint32]0
        [void][Win32CaptureBounds]::GetWindowThreadProcessId($hWnd, [ref]$windowPid)
        $script:rows.Add([pscustomobject]@{
            Handle = $hWnd
            Pid = $windowPid
            Class = $classSb.ToString()
            Title = $titleSb.ToString()
            Width = ($rect.Right - $rect.Left)
            Height = ($rect.Bottom - $rect.Top)
        })
        return $true
    }, [IntPtr]::Zero) | Out-Null
    $rows | Format-Table -AutoSize
    exit 0
}

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

$resolvedEvidenceFolder = (Resolve-Path -LiteralPath $EvidenceFolder).Path
if (-not $OutputPath) {
    $OutputPath = Join-Path $resolvedEvidenceFolder $OutputName
}

if ([System.IO.Path]::GetExtension($OutputPath).ToLowerInvariant() -ne ".png") {
    throw "Screenshot OutputPath must use a .png extension: $OutputPath"
}

$outputParent = Split-Path -Parent $OutputPath
if (-not $outputParent) {
    $outputParent = $resolvedEvidenceFolder
    $OutputPath = Join-Path $outputParent $OutputPath
}
if (-not (Test-Path $outputParent -PathType Container)) {
    throw "Screenshot output directory does not exist: $outputParent"
}

$resolvedOutputParent = (Resolve-Path -LiteralPath $outputParent).Path
$evidenceRootForCompare = [System.IO.Path]::GetFullPath($resolvedEvidenceFolder).TrimEnd("\")
$outputParentForCompare = [System.IO.Path]::GetFullPath($resolvedOutputParent).TrimEnd("\")
if (
    $outputParentForCompare -ne $evidenceRootForCompare -and
    -not $outputParentForCompare.StartsWith($evidenceRootForCompare + "\", [System.StringComparison]::OrdinalIgnoreCase)
) {
    throw "Screenshot output must stay inside the evidence folder: $resolvedEvidenceFolder"
}

if ($DryRun) {
    Write-Host "[OK] FW MVP PIE screenshot capture dry-run passed." -ForegroundColor Green
    Write-Host "Evidence folder: $resolvedEvidenceFolder"
    Write-Host "Output path: $OutputPath"
    if ($WindowOnly) {
        $targetProcess = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue | Sort-Object StartTime -Descending | Select-Object -First 1
        if (-not $targetProcess) {
            Write-Host "[WARN] WindowOnly capture target process was not found by name; falling back to window class/title search: $ProcessName" -ForegroundColor Yellow
        }
        $targetWindow = Get-CaptureWindowHandle $targetProcess
        if ($targetWindow -eq [IntPtr]::Zero) {
            throw "WindowOnly capture target process has no main window handle: $ProcessName"
        }
        if ($targetProcess) {
            Write-Host "Window-only process: $ProcessName ($($targetProcess.Id))"
        }
        Write-Host "Window-only handle: $targetWindow"
        if ($SkipPrintWindow) {
            Write-Host "Window-only capture mode: CopyFromScreen"
        }
    }
    exit 0
}

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$bounds = [System.Windows.Forms.SystemInformation]::VirtualScreen
if ($WindowOnly) {
    $targetProcess = Get-Process -Name $ProcessName -ErrorAction SilentlyContinue | Sort-Object StartTime -Descending | Select-Object -First 1
    if (-not $targetProcess) {
        Write-Host "[WARN] WindowOnly capture target process was not found by name; falling back to window class/title search: $ProcessName" -ForegroundColor Yellow
    }
    $mainWindow = Get-CaptureWindowHandle $targetProcess
    if ($mainWindow -eq [IntPtr]::Zero) {
        throw "WindowOnly capture target process has no capturable window handle: $ProcessName"
    }
    Set-CaptureWindowForeground -WindowHandle $mainWindow
    Start-Sleep -Milliseconds 800
    $rect = New-Object Win32CaptureBounds+RECT
    if (-not [Win32CaptureBounds]::GetWindowRect($mainWindow, [ref]$rect)) {
        throw "Failed to read window bounds for process: $ProcessName"
    }
    $bounds = New-Object System.Drawing.Rectangle $rect.Left, $rect.Top, ($rect.Right - $rect.Left), ($rect.Bottom - $rect.Top)
}
if ($bounds.Width -le 0 -or $bounds.Height -le 0) {
    throw "Virtual screen has invalid bounds: $($bounds.Width)x$($bounds.Height)"
}

$bitmap = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
try {
    $printedWindow = $false
    if ($WindowOnly -and -not $SkipPrintWindow -and $mainWindow -and $mainWindow -ne [IntPtr]::Zero) {
        $hdc = $graphics.GetHdc()
        try {
            $printedWindow = [Win32CaptureBounds]::PrintWindow($mainWindow, $hdc, 2)
        }
        finally {
            $graphics.ReleaseHdc($hdc)
        }
    }
    if (-not $printedWindow) {
        $graphics.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
    }
    $bitmap.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Png)
}
finally {
    $graphics.Dispose()
    $bitmap.Dispose()
}

$savedFile = Get-Item -LiteralPath $OutputPath
if ($savedFile.Length -lt 1024) {
    throw "Captured screenshot is too small to be useful evidence: $($savedFile.FullName) ($($savedFile.Length) bytes)"
}

$stream = [System.IO.File]::OpenRead($savedFile.FullName)
try {
    $signature = New-Object byte[] 8
    $bytesRead = $stream.Read($signature, 0, $signature.Length)
    if (
        $bytesRead -ne 8 -or
        $signature[0] -ne 0x89 -or
        $signature[1] -ne 0x50 -or
        $signature[2] -ne 0x4E -or
        $signature[3] -ne 0x47 -or
        $signature[4] -ne 0x0D -or
        $signature[5] -ne 0x0A -or
        $signature[6] -ne 0x1A -or
        $signature[7] -ne 0x0A
    ) {
        throw "Captured screenshot does not have a valid PNG signature: $($savedFile.FullName)"
    }
}
finally {
    $stream.Dispose()
}

Write-Host "[OK] FW MVP PIE screenshot captured: $($savedFile.FullName) ($($savedFile.Length) bytes)" -ForegroundColor Green
