param(
    [string]$ReportPath = (Join-Path (Split-Path -Parent $PSScriptRoot) "docs\FW_MVP_PIE_Acceptance_Report.md"),
    [switch]$AllowIncomplete
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $ReportPath)) {
    throw "PIE acceptance report not found: $ReportPath"
}

$reportText = Get-Content -Raw $ReportPath
$requiredFields = @(
    "Date:",
    "Tester:",
    "Screenshot or capture folder:",
    "Overall status:",
    "Blocking issues:"
)

$missingFields = @()
foreach ($field in $requiredFields) {
    if ($reportText -notmatch [regex]::Escape($field)) {
        $missingFields += $field
    }
}

$placeholderLines = Select-String -Path $ReportPath -Pattern ":\s*TBD\s*$"
$uncheckedItems = Select-String -Path $ReportPath -Pattern "^\s*-\s+\[\s\]\s+PIE-\d+"
$checkedItems = Select-String -Path $ReportPath -Pattern "^\s*-\s+\[[xX]\]\s+PIE-\d+"
$requiredItemCount = 24
$minimumEvidenceMediaBytes = 1024
$evidenceFolder = $null
if ($reportText -match "Screenshot or capture folder:\s*(.+)") {
    $evidenceFolder = $Matches[1].Trim()
}

function Get-EvidenceMediaKind {
    param(
        [System.IO.FileInfo]$File
    )

    if ($File.Length -lt 12) {
        return $null
    }

    $stream = [System.IO.File]::OpenRead($File.FullName)
    try {
        $buffer = New-Object byte[] 16
        $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
        if ($bytesRead -lt 12) {
            return $null
        }

        switch ($File.Extension.ToLowerInvariant()) {
            ".png" {
                if (
                    $buffer[0] -eq 0x89 -and
                    $buffer[1] -eq 0x50 -and
                    $buffer[2] -eq 0x4E -and
                    $buffer[3] -eq 0x47 -and
                    $buffer[4] -eq 0x0D -and
                    $buffer[5] -eq 0x0A -and
                    $buffer[6] -eq 0x1A -and
                    $buffer[7] -eq 0x0A
                ) {
                    return "PNG"
                }
                return $null
            }
            ".jpg" {
                if ($buffer[0] -eq 0xFF -and $buffer[1] -eq 0xD8) {
                    return "JPEG"
                }
                return $null
            }
            ".jpeg" {
                if ($buffer[0] -eq 0xFF -and $buffer[1] -eq 0xD8) {
                    return "JPEG"
                }
                return $null
            }
            ".mp4" {
                if ([System.Text.Encoding]::ASCII.GetString($buffer, 4, 4) -eq "ftyp") {
                    return "MP4"
                }
                return $null
            }
            ".mov" {
                if ([System.Text.Encoding]::ASCII.GetString($buffer, 4, 4) -eq "ftyp") {
                    return "MOV"
                }
                return $null
            }
            ".webm" {
                if (
                    $buffer[0] -eq 0x1A -and
                    $buffer[1] -eq 0x45 -and
                    $buffer[2] -eq 0xDF -and
                    $buffer[3] -eq 0xA3
                ) {
                    return "WebM"
                }
                return $null
            }
            default { return $null }
        }
    }
    finally {
        $stream.Dispose()
    }
}

$issues = @()
$acceptedEvidenceMedia = @()
$acceptedGates = @()
$acceptedMetadata = [ordered]@{}
$acceptedRuntimeEvidence = $null
if ($missingFields.Count -gt 0) {
    $issues += "Missing required field(s): $($missingFields -join ', ')"
}
if ($checkedItems.Count -lt $requiredItemCount) {
    $issues += "Expected $requiredItemCount checked PIE item(s), found $($checkedItems.Count)."
}
if ($uncheckedItems.Count -gt 0) {
    $issues += "Unchecked PIE item(s): $($uncheckedItems.Count)."
}
if ($placeholderLines.Count -gt 0) {
    $issues += "Unresolved TBD field(s): $($placeholderLines.Count)."
}
if ($reportText -notmatch "Overall status:\s*Pass") {
    $issues += "Overall status must be 'Pass'."
}
if ($reportText -notmatch "Blocking issues:\s*None") {
    $issues += "Blocking issues must be 'None'."
}
if (-not $evidenceFolder -or $evidenceFolder -eq "TBD") {
    $issues += "Screenshot or capture folder must point to an evidence directory."
}
elseif (-not (Test-Path $evidenceFolder -PathType Container)) {
    $issues += "Evidence directory does not exist: $evidenceFolder"
}
else {
    $evidenceFiles = Get-ChildItem -LiteralPath $evidenceFolder -Recurse -File | Where-Object {
        $_.Extension -in @(".png", ".jpg", ".jpeg", ".mp4", ".mov", ".webm")
    }
    foreach ($evidenceFile in $evidenceFiles) {
        $mediaKind = Get-EvidenceMediaKind $evidenceFile
        if ($evidenceFile.Length -ge $minimumEvidenceMediaBytes -and $mediaKind) {
            $mediaHash = Get-FileHash -LiteralPath $evidenceFile.FullName -Algorithm SHA256
            $acceptedEvidenceMedia += [ordered]@{
                Path = $evidenceFile.FullName
                Kind = $mediaKind
                Bytes = $evidenceFile.Length
                SHA256 = $mediaHash.Hash
            }
        }
    }
    if ($evidenceFiles.Count -eq 0) {
        $issues += "Evidence directory must contain at least one screenshot or video file."
    }
    elseif ($acceptedEvidenceMedia.Count -eq 0) {
        $issues += "Evidence screenshot or video files must include at least one valid PNG/JPEG/MP4/MOV/WebM file >= $minimumEvidenceMediaBytes bytes."
    }

    $readmePath = Join-Path $evidenceFolder "README.md"
    if (-not (Test-Path $readmePath -PathType Leaf)) {
        $issues += "Evidence directory must contain README.md from New-FWMVPPIEAcceptanceSession.ps1."
    }
    elseif ((Get-Item -LiteralPath $readmePath).Length -eq 0) {
        $issues += "Evidence README.md is empty."
    }

    $runtimeEvidencePath = Join-Path $evidenceFolder "PIE-runtime-evidence.log"
    if (-not (Test-Path $runtimeEvidencePath -PathType Leaf)) {
        $issues += "Evidence directory must contain PIE-runtime-evidence.log from Export-FWMVPPIERuntimeEvidence.ps1."
    }
    elseif ((Get-Item -LiteralPath $runtimeEvidencePath).Length -eq 0) {
        $issues += "PIE runtime evidence log is empty."
    }
    else {
        $runtimeEvidenceText = Get-Content -Raw $runtimeEvidencePath
        $requiredRuntimeEvidence = @(
            "LogDebuggerCommands: Repeating last play command: Selected Viewport",
            "LogPlayLevel: Creating play world package: /Game/FW/Maps/Test/UEDPIE_0_L_Test_CombatGarage",
            "LogPlayLevel: PIE: Created PIE world by copying editor world from /Game/FW/Maps/Test/L_Test_CombatGarage",
            "LogLoad: Game class is 'FWCompetitiveGameMode'",
            "LogWorld: Bringing World /Game/FW/Maps/Test/UEDPIE_0_L_Test_CombatGarage"
        )
        foreach ($requiredRuntimeText in $requiredRuntimeEvidence) {
            if ($runtimeEvidenceText -notmatch [regex]::Escape($requiredRuntimeText)) {
                $issues += "PIE runtime evidence log is missing required signal: $requiredRuntimeText"
            }
        }
        if ($issues.Count -eq 0 -or ($requiredRuntimeEvidence | Where-Object { $runtimeEvidenceText -notmatch [regex]::Escape($_) }).Count -eq 0) {
            $runtimeHash = Get-FileHash -LiteralPath $runtimeEvidencePath -Algorithm SHA256
            $acceptedRuntimeEvidence = [ordered]@{
                Path = $runtimeEvidencePath
                Bytes = (Get-Item -LiteralPath $runtimeEvidencePath).Length
                SHA256 = $runtimeHash.Hash
            }
        }
    }

    $checklistLedgerPath = Join-Path $evidenceFolder "PIE-checklist-evidence.json"
    if (-not (Test-Path $checklistLedgerPath -PathType Leaf)) {
        $issues += "Evidence directory must contain PIE-checklist-evidence.json from Set-FWMVPPIEChecklistItem.ps1."
    }
    else {
        $checklistLedger = Get-Content -Raw $checklistLedgerPath | ConvertFrom-Json
        $ledgerItems = @($checklistLedger.Items)
        $requiredChecklistIds = 0..23 | ForEach-Object { "PIE-{0:D2}" -f $_ }
        foreach ($requiredChecklistId in $requiredChecklistIds) {
            $ledgerItem = @($ledgerItems | Where-Object { $_.ItemId -eq $requiredChecklistId }) | Select-Object -First 1
            if (-not $ledgerItem) {
                $issues += "PIE checklist evidence ledger is missing item: $requiredChecklistId"
            }
            elseif (-not $ledgerItem.CheckedAt -or -not $ledgerItem.Tester -or -not $ledgerItem.EvidenceNote -or -not $ledgerItem.ReportLine) {
                $issues += "PIE checklist evidence ledger item is incomplete: $requiredChecklistId"
            }
        }
    }

    $manifestPath = Join-Path $evidenceFolder "manifest.json"
    if (-not (Test-Path $manifestPath -PathType Leaf)) {
        $issues += "Evidence directory must contain manifest.json from New-FWMVPPIEAcceptanceSession.ps1."
    }
    else {
        $manifest = Get-Content -Raw $manifestPath | ConvertFrom-Json
        if (-not $manifest.CreatedAt) {
            $issues += "PIE acceptance session manifest is missing CreatedAt."
        }
        else {
            $acceptedMetadata.CreatedAt = $manifest.CreatedAt
        }
        if (-not $manifest.Tester) {
            $issues += "PIE acceptance session manifest is missing Tester."
        }
        else {
            $acceptedMetadata.Tester = $manifest.Tester
        }
        if (-not $manifest.ProjectFile) {
            $issues += "PIE acceptance session manifest is missing ProjectFile."
        }
        elseif (-not (Test-Path $manifest.ProjectFile -PathType Leaf)) {
            $issues += "PIE acceptance session manifest ProjectFile does not exist: $($manifest.ProjectFile)"
        }
        elseif ([System.IO.Path]::GetExtension([string]$manifest.ProjectFile).ToLowerInvariant() -ne ".uproject") {
            $issues += "PIE acceptance session manifest ProjectFile must point to a .uproject file: $($manifest.ProjectFile)"
        }
        else {
            $acceptedMetadata.ProjectFile = (Resolve-Path -LiteralPath $manifest.ProjectFile).Path
        }
        if ($manifest.SkipPreflight) {
            $issues += "PIE acceptance session manifest was created with SkipPreflight; run a preflight session before final acceptance."
        }
        if (-not $manifest.SessionDir) {
            $issues += "PIE acceptance session manifest is missing SessionDir."
        }
        else {
            $resolvedEvidenceFolder = (Resolve-Path -LiteralPath $evidenceFolder).Path
            if (-not (Test-Path $manifest.SessionDir -PathType Container)) {
                $issues += "PIE acceptance session manifest SessionDir does not exist: $($manifest.SessionDir)"
            }
            else {
                $resolvedSessionDir = (Resolve-Path -LiteralPath $manifest.SessionDir).Path
                if ($resolvedSessionDir -ne $resolvedEvidenceFolder) {
                    $issues += "PIE acceptance session manifest SessionDir does not match the report evidence folder."
                }
            }
        }

        $requiredGates = @("Build", "AssetBootstrap", "MVPVerify", "RuntimeVerify", "FullLoopVerify", "SmokeTest", "Readiness", "PIELaunchDryRun")
        foreach ($gateName in $requiredGates) {
            $gate = @($manifest.Gates | Where-Object { $_.Name -eq $gateName }) | Select-Object -First 1
            if (-not $gate) {
                $issues += "PIE acceptance manifest is missing gate: $gateName"
            }
            elseif ($gate.Status -ne "Passed" -or $gate.ExitCode -ne 0) {
                $issues += "PIE acceptance manifest gate did not pass: $gateName"
            }
            elseif (-not $gate.OutputLog) {
                $issues += "PIE acceptance manifest gate is missing OutputLog: $gateName"
            }
            elseif (-not (Test-Path $gate.OutputLog -PathType Leaf)) {
                $issues += "PIE acceptance manifest gate OutputLog does not exist for ${gateName}: $($gate.OutputLog)"
            }
            elseif ((Get-Item -LiteralPath $gate.OutputLog).Length -eq 0) {
                $issues += "PIE acceptance manifest gate OutputLog is empty: $gateName"
            }
            else {
                $acceptedGates += [ordered]@{
                    Name = $gateName
                    OutputLog = $gate.OutputLog
                }
            }
        }
    }
}

if ($issues.Count -gt 0) {
    $prefix = if ($AllowIncomplete) { "WARN" } else { "FAIL" }
    $color = if ($AllowIncomplete) { "Yellow" } else { "Red" }
    Write-Host "[$prefix] FW MVP PIE acceptance is incomplete:" -ForegroundColor $color
    $issues | ForEach-Object { Write-Host "  - $_" -ForegroundColor $color }
    if (-not $AllowIncomplete) {
        exit 1
    }
}
else {
    Write-Host "[OK] FW MVP PIE acceptance report is complete." -ForegroundColor Green
    Write-Host "[OK] Accepted session metadata:" -ForegroundColor Green
    $acceptedMetadata.GetEnumerator() | ForEach-Object {
        Write-Host "  - $($_.Key): $($_.Value)" -ForegroundColor Green
    }
    Write-Host "[OK] Accepted media evidence:" -ForegroundColor Green
    $acceptedEvidenceMedia | ForEach-Object {
        Write-Host "  - $($_.Kind): $($_.Path) ($($_.Bytes) bytes, SHA256: $($_.SHA256))" -ForegroundColor Green
    }
    Write-Host "[OK] Accepted PIE runtime evidence:" -ForegroundColor Green
    Write-Host "  - $($acceptedRuntimeEvidence.Path) ($($acceptedRuntimeEvidence.Bytes) bytes, SHA256: $($acceptedRuntimeEvidence.SHA256))" -ForegroundColor Green
    Write-Host "[OK] Accepted manifest gates:" -ForegroundColor Green
    $acceptedGates | ForEach-Object {
        Write-Host "  - $($_.Name): $($_.OutputLog)" -ForegroundColor Green
    }
}
