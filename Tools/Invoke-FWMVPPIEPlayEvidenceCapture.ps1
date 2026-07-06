param(
    [string]$EvidenceFolder,
    [string]$ReportPath = (Join-Path (Split-Path -Parent $PSScriptRoot) "docs\FW_MVP_PIE_Acceptance_Report.md"),
    [string]$OutputName = ("FW_MVP_PIE_PlayEvidence_{0}.png" -f (Get-Date -Format "yyyyMMdd-HHmmss")),
    [int]$EditorReadyTimeoutSeconds = 300,
    [int]$WarmupSeconds = 20,
    [switch]$SkipInput,
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
$repoRoot = Split-Path -Parent $PSScriptRoot
$editorLog = Join-Path $repoRoot "Saved\Logs\FW.log"
if (-not (Test-Path $captureScript -PathType Leaf)) {
    throw "Screenshot capture script not found: $captureScript"
}

if ($DryRun) {
    Write-Host "[OK] FW MVP PIE play evidence dry-run passed." -ForegroundColor Green
    Write-Host "Evidence folder: $((Resolve-Path -LiteralPath $EvidenceFolder).Path)"
    Write-Host "Output name: $OutputName"
    Write-Host "Editor ready timeout seconds: $EditorReadyTimeoutSeconds"
    Write-Host "Warmup seconds: $WarmupSeconds"
    Write-Host "Skip input: $([bool]$SkipInput)"
    exit 0
}

$editorProcess = Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue | Sort-Object StartTime -Descending | Select-Object -First 1
if (-not $editorProcess) {
    throw "UnrealEditor is not running. Launch it with Tools\Open-FWMVPCombatGaragePIE.ps1 first."
}

Add-Type -AssemblyName System.Windows.Forms
Add-Type @'
using System;
using System.Runtime.InteropServices;
public class Win32Focus {
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
}
public class Win32Input {
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int X, int Y);
    [DllImport("user32.dll")] public static extern void mouse_event(uint dwFlags, uint dx, uint dy, uint dwData, UIntPtr dwExtraInfo);
    [DllImport("user32.dll")] public static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, UIntPtr dwExtraInfo);
}
'@

function Get-EditorWindowHandle {
    param([System.Diagnostics.Process]$Process)

    $targetPid = if ($Process) { [uint32]$Process.Id } else { [uint32]0 }
    $script:editorWindowCandidate = [IntPtr]::Zero
    $script:editorWindowScore = [int64]::MinValue
    [Win32Focus]::EnumWindows({
        param($hWnd, $lParam)

        $windowPid = [uint32]0
        [void][Win32Focus]::GetWindowThreadProcessId($hWnd, [ref]$windowPid)
        if ($targetPid -ne 0 -and $windowPid -ne $targetPid) {
            return $true
        }

        $rect = New-Object Win32Focus+RECT
        if (-not [Win32Focus]::GetWindowRect($hWnd, [ref]$rect)) {
            return $true
        }

        $width = $rect.Right - $rect.Left
        $height = $rect.Bottom - $rect.Top
        if ($width -lt 320 -or $height -lt 240) {
            return $true
        }

        $classSb = New-Object System.Text.StringBuilder 256
        $titleSb = New-Object System.Text.StringBuilder 512
        [void][Win32Focus]::GetClassName($hWnd, $classSb, $classSb.Capacity)
        [void][Win32Focus]::GetWindowText($hWnd, $titleSb, $titleSb.Capacity)
        $className = $classSb.ToString()
        $title = $titleSb.ToString()
        if ($className -eq "ConsoleWindowClass" -or $title -like "*.exe*") {
            return $true
        }
        if (-not [Win32Focus]::IsWindowVisible($hWnd) -and $className -ne "UnrealWindow") {
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
    $swpNoMove = 0x0002
    $swpNoSize = 0x0001
    $swpShowWindow = 0x0040
    $flags = $swpNoMove -bor $swpNoSize -bor $swpShowWindow

    [void][Win32Focus]::ShowWindowAsync($WindowHandle, 3)
    [void][Win32Focus]::SetWindowPos($WindowHandle, $hwndTopMost, 0, 0, 0, 0, $flags)
    [void][Win32Focus]::BringWindowToTop($WindowHandle)
    [void][Win32Focus]::SetForegroundWindow($WindowHandle)
    Start-Sleep -Milliseconds 250
    [void][Win32Focus]::SetWindowPos($WindowHandle, $hwndNoTopMost, 0, 0, 0, 0, $flags)
}

$readyDeadline = (Get-Date).AddSeconds($EditorReadyTimeoutSeconds)
$mainWindow = [IntPtr]::Zero
while ((Get-Date) -lt $readyDeadline) {
    $mainWindow = Get-EditorWindowHandle $editorProcess
    if ($mainWindow -ne [IntPtr]::Zero) {
        break
    }
    Start-Sleep -Seconds 2
}
if ($mainWindow -eq [IntPtr]::Zero) {
    throw "UnrealEditor main window handle is not available before timeout ($EditorReadyTimeoutSeconds seconds)."
}

$editorReady = $false
while ((Get-Date) -lt $readyDeadline) {
    if (Test-Path $editorLog -PathType Leaf) {
        $logText = Get-Content -LiteralPath $editorLog -Tail 250 -ErrorAction SilentlyContinue
        if (($logText | Select-String -Pattern "LogLoad: \(Engine Initialization\) Total time:|MapCheck: Map check complete" -Quiet)) {
            $editorReady = $true
            break
        }
    }
    Start-Sleep -Seconds 2
}

if (-not $editorReady) {
    throw "UnrealEditor did not finish initialization before timeout ($EditorReadyTimeoutSeconds seconds)."
}

Set-EditorWindowForeground -WindowHandle $mainWindow
Start-Sleep -Milliseconds 700

$initialLogLength = 0
if (Test-Path $editorLog -PathType Leaf) {
    $initialLogLength = (Get-Item -LiteralPath $editorLog).Length
}

$pieStarted = $false
$deadline = (Get-Date).AddSeconds($WarmupSeconds)

function Read-NewEditorLogText {
    if (-not (Test-Path $editorLog -PathType Leaf)) {
        return ""
    }

    $currentLog = Get-Item -LiteralPath $editorLog
    if ($currentLog.Length -le $initialLogLength) {
        return ""
    }

    $stream = [System.IO.File]::Open($editorLog, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
    try {
        $stream.Seek($initialLogLength, [System.IO.SeekOrigin]::Begin) | Out-Null
        $reader = New-Object System.IO.StreamReader($stream)
        return $reader.ReadToEnd()
    }
    finally {
        if ($reader) { $reader.Dispose() }
        $stream.Dispose()
    }
}

function Test-PIEStarted {
    Start-Sleep -Seconds 1
    $newText = Read-NewEditorLogText
    return ($newText -match "LogPlayLevel:.*Created PIE world|LogWorld: Bringing World .* up for play")
}

function Test-PIEStartedInLogTail {
    if (-not (Test-Path $editorLog -PathType Leaf)) {
        return $false
    }

    $tailText = (Get-Content -LiteralPath $editorLog -Tail 500 -ErrorAction SilentlyContinue) -join "`n"
    return ($tailText -match "LogPlayLevel:.*Created PIE world|LogWorld: Bringing World .* up for play")
}

function Invoke-MouseClick {
    param(
        [int]$X,
        [int]$Y
    )

    [void][Win32Input]::SetCursorPos($X, $Y)
    Start-Sleep -Milliseconds 150
    [Win32Input]::mouse_event(0x0002, 0, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 80
    [Win32Input]::mouse_event(0x0004, 0, 0, 0, [UIntPtr]::Zero)
}

function Invoke-AltP {
    [Win32Input]::keybd_event(0x12, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 80
    [Win32Input]::keybd_event(0x50, 0, 0, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 80
    [Win32Input]::keybd_event(0x50, 0, 0x0002, [UIntPtr]::Zero)
    Start-Sleep -Milliseconds 80
    [Win32Input]::keybd_event(0x12, 0, 0x0002, [UIntPtr]::Zero)
}

$windowRect = New-Object Win32Input+RECT
$pieStarted = Test-PIEStartedInLogTail

if ((-not $pieStarted) -and (-not $SkipInput) -and [Win32Input]::GetWindowRect($mainWindow, [ref]$windowRect)) {
    $windowWidth = [Math]::Max(1, $windowRect.Right - $windowRect.Left)
    $windowHeight = [Math]::Max(1, $windowRect.Bottom - $windowRect.Top)
    $playX = $windowRect.Left + [int]($windowWidth * 0.585)
    $playY = $windowRect.Top + [int]($windowHeight * 0.178)
    $focusX = $windowRect.Left + [int]($windowWidth * 0.5)
    $focusY = $windowRect.Top + [int]($windowHeight * 0.5)
    Invoke-MouseClick -X $focusX -Y $focusY
    Start-Sleep -Milliseconds 400
    Invoke-MouseClick -X $playX -Y $playY
    Start-Sleep -Milliseconds 450
    Invoke-MouseClick -X $playX -Y $playY
}

$clickDeadline = (Get-Date).AddSeconds([Math]::Min(18, $WarmupSeconds))
while ((Get-Date) -lt $clickDeadline) {
    if (Test-PIEStarted) {
        $pieStarted = $true
        break
    }
}

if ((-not $pieStarted) -and (-not $SkipInput)) {
    Set-EditorWindowForeground -WindowHandle $mainWindow
    Start-Sleep -Milliseconds 300
    Invoke-AltP
}

while ((-not $pieStarted) -and (Get-Date) -lt $deadline) {
    if (Test-PIEStarted) {
        $pieStarted = $true
        break
    }
}

if ($pieStarted) {
    Write-Host "[OK] UE log indicates PIE started." -ForegroundColor Green
    Start-Sleep -Seconds 3
}
else {
    Write-Host "[WARN] PIE start log was not observed before capture; taking screenshot after warmup timeout." -ForegroundColor Yellow
}

& powershell -NoProfile -ExecutionPolicy Bypass -File $captureScript -EvidenceFolder $EvidenceFolder -OutputName $OutputName -WindowOnly -ProcessName "UnrealEditor" -SkipPrintWindow
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "[OK] FW MVP PIE play evidence capture attempted." -ForegroundColor Green
