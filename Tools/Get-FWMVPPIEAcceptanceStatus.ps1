param(
    [string]$ReportPath = (Join-Path (Split-Path -Parent $PSScriptRoot) "docs\FW_MVP_PIE_Acceptance_Report.md"),
    [string]$Tester
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $ReportPath -PathType Leaf)) {
    throw "PIE acceptance report not found: $ReportPath"
}

$repoRoot = Split-Path -Parent $PSScriptRoot
$reportText = Get-Content -Raw $ReportPath
if (-not $Tester) {
    if ($reportText -match "(?m)^- Tester:\s*(.+)$") {
        $Tester = $Matches[1].Trim()
    }
    if (-not $Tester -or $Tester -eq "TBD") {
        $Tester = $env:USERNAME
    }
}
$requiredItemCount = 24
$requiredGates = @("Build", "AssetBootstrap", "MVPVerify", "RuntimeVerify", "FullLoopVerify", "SmokeTest", "Readiness", "PIELaunchDryRun")
$validMediaExtensions = @(".png", ".jpg", ".jpeg", ".mp4", ".mov", ".webm")

$evidenceFolder = $null
if ($reportText -match "Screenshot or capture folder:\s*(.+)") {
    $evidenceFolder = $Matches[1].Trim()
}

$checkedCount = (Select-String -Path $ReportPath -Pattern "^\s*-\s+\[[xX]\]\s+PIE-\d+").Count
$uncheckedCount = (Select-String -Path $ReportPath -Pattern "^\s*-\s+\[\s\]\s+PIE-\d+").Count
$tbdCount = (Select-String -Path $ReportPath -Pattern ":\s*TBD\s*$").Count
$candidateMedia = @()
$manifestGatePassedCount = 0
$manifestGateIssueCount = 0
$resolvedEvidenceFolder = $null
$runtimeEvidenceStatus = "Missing"
$checklistLedgerCount = 0
$nextMissingChecklistId = "PIE-00"

if ($evidenceFolder -and $evidenceFolder -ne "TBD" -and (Test-Path $evidenceFolder -PathType Container)) {
    $resolvedEvidenceFolder = (Resolve-Path -LiteralPath $evidenceFolder).Path
    $candidateMedia = @(Get-ChildItem -LiteralPath $resolvedEvidenceFolder -Recurse -File | Where-Object {
        $_.Extension.ToLowerInvariant() -in $validMediaExtensions -and $_.Length -ge 1024
    })

    $manifestPath = Join-Path $resolvedEvidenceFolder "manifest.json"
    $runtimeEvidencePath = Join-Path $resolvedEvidenceFolder "PIE-runtime-evidence.log"
    $checklistLedgerPath = Join-Path $resolvedEvidenceFolder "PIE-checklist-evidence.json"
    if (Test-Path $runtimeEvidencePath -PathType Leaf) {
        $runtimeEvidenceText = Get-Content -Raw $runtimeEvidencePath
        if (
            $runtimeEvidenceText -match [regex]::Escape("LogPlayLevel: Creating play world package: /Game/FW/Maps/Test/UEDPIE_0_L_Test_CombatGarage") -and
            $runtimeEvidenceText -match [regex]::Escape("LogLoad: Game class is 'FWCompetitiveGameMode'") -and
            $runtimeEvidenceText -match [regex]::Escape("LogWorld: Bringing World /Game/FW/Maps/Test/UEDPIE_0_L_Test_CombatGarage")
        ) {
            $runtimeEvidenceStatus = "Present"
        }
        else {
            $runtimeEvidenceStatus = "Invalid"
        }
    }
    if (Test-Path $checklistLedgerPath -PathType Leaf) {
        $checklistLedger = Get-Content -Raw $checklistLedgerPath | ConvertFrom-Json
        $ledgerItems = @($checklistLedger.Items)
        $checklistLedgerCount = $ledgerItems.Count
        foreach ($candidateId in (0..23 | ForEach-Object { "PIE-{0:D2}" -f $_ })) {
            if (-not (@($ledgerItems | Where-Object { $_.ItemId -eq $candidateId }) | Select-Object -First 1)) {
                $nextMissingChecklistId = $candidateId
                break
            }
        }
    }

    $manifestPath = Join-Path $resolvedEvidenceFolder "manifest.json"
    if (Test-Path $manifestPath -PathType Leaf) {
        $manifest = Get-Content -Raw $manifestPath | ConvertFrom-Json
        foreach ($gateName in $requiredGates) {
            $gate = @($manifest.Gates | Where-Object { $_.Name -eq $gateName }) | Select-Object -First 1
            if ($gate -and $gate.Status -eq "Passed" -and $gate.ExitCode -eq 0 -and $gate.OutputLog -and (Test-Path $gate.OutputLog -PathType Leaf) -and (Get-Item -LiteralPath $gate.OutputLog).Length -gt 0) {
                $manifestGatePassedCount += 1
            }
            else {
                $manifestGateIssueCount += 1
            }
        }
    }
    else {
        $manifestGateIssueCount = $requiredGates.Count
    }
}
else {
    $manifestGateIssueCount = $requiredGates.Count
}

$validator = Join-Path $PSScriptRoot "Test-FWMVPPIEAcceptance.ps1"
$previousErrorActionPreference = $ErrorActionPreference
$ErrorActionPreference = "Continue"
try {
    $validationOutput = & powershell -NoProfile -ExecutionPolicy Bypass -File $validator -ReportPath $ReportPath -AllowIncomplete *>&1
    $validationExitCode = if ($null -ne $LASTEXITCODE) { $LASTEXITCODE } else { 0 }
}
finally {
    $ErrorActionPreference = $previousErrorActionPreference
}

$complete = (
    $validationExitCode -eq 0 -and
    (($validationOutput | Out-String) -match "\[OK\] FW MVP PIE acceptance report is complete\.")
)

Write-Host "[INFO] FW MVP PIE acceptance status"
Write-Host "  - Report: $ReportPath"
Write-Host "  - Evidence folder: $(if ($resolvedEvidenceFolder) { $resolvedEvidenceFolder } else { $evidenceFolder })"
Write-Host "  - PIE checklist: $checkedCount/$requiredItemCount checked, $uncheckedCount unchecked"
Write-Host "  - TBD fields: $tbdCount"
Write-Host "  - Candidate media files: $($candidateMedia.Count)"
Write-Host "  - PIE runtime evidence: $runtimeEvidenceStatus"
Write-Host "  - Checklist evidence ledger: $checklistLedgerCount/$requiredItemCount"
Write-Host "  - Manifest gates: $manifestGatePassedCount/$($requiredGates.Count) passed"

if ($complete) {
    Write-Host "[OK] FW MVP PIE acceptance is complete." -ForegroundColor Green
    Write-Host "[NEXT] Run final readiness gate before commit/push:"
    Write-Host "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Test-FWMVPReadiness.ps1 -StrictAssets -VerifyPIEValidator -RequirePIEAcceptance"
    exit 0
}

Write-Host "[WARN] FW MVP PIE acceptance is not complete." -ForegroundColor Yellow
if (-not $resolvedEvidenceFolder) {
    Write-Host "[NEXT] Create or select a PIE acceptance session:"
    Write-Host "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\New-FWMVPPIEAcceptanceSession.ps1 -ProjectFile `"$repoRoot\FW.uproject`" -Tester `"$Tester`""
}
elseif ($manifestGateIssueCount -gt 0) {
    Write-Host "[NEXT] Recreate the PIE acceptance session so every preflight gate is recorded as passed:"
    Write-Host "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\New-FWMVPPIEAcceptanceSession.ps1 -ProjectFile `"$repoRoot\FW.uproject`" -Tester `"$Tester`""
}
elseif ($candidateMedia.Count -eq 0) {
    Write-Host "[NEXT] Open manual PIE and capture at least one PNG/JPEG/MP4/MOV/WebM evidence file >= 1024 bytes into:"
    Write-Host $resolvedEvidenceFolder
    Write-Host "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Open-FWMVPCombatGaragePIE.ps1 -ProjectFile `"$repoRoot\FW.uproject`" -StartPIE"
    Write-Host "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Invoke-FWMVPPIEPlayEvidenceCapture.ps1 -EvidenceFolder `"$resolvedEvidenceFolder`" -SkipInput"
    Write-Host "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Capture-FWMVPPIEEvidenceScreenshot.ps1 -EvidenceFolder `"$resolvedEvidenceFolder`""
}
elseif ($runtimeEvidenceStatus -ne "Present") {
    Write-Host "[NEXT] Export PIE runtime log evidence:"
    Write-Host "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Export-FWMVPPIERuntimeEvidence.ps1 -EvidenceFolder `"$resolvedEvidenceFolder`""
}
elseif ($checklistLedgerCount -lt $requiredItemCount) {
    Write-Host "[NEXT] Record each manually verified checklist item as it is completed:"
    Write-Host "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Set-FWMVPPIEChecklistItem.ps1 -ItemId $nextMissingChecklistId -EvidenceNote `"Manual observation note here`""
}

if ($resolvedEvidenceFolder -and $candidateMedia.Count -gt 0 -and $runtimeEvidenceStatus -eq "Present" -and $checklistLedgerCount -ge $requiredItemCount -and ($checkedCount -lt $requiredItemCount -or $uncheckedCount -gt 0 -or $tbdCount -gt 0)) {
    Write-Host "[NEXT] After manual PIE is truly complete, finalize the report:"
    Write-Host "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Complete-FWMVPPIEAcceptanceReport.ps1 -EvidenceFolder `"$resolvedEvidenceFolder`" -Tester `"$Tester`" -ConfirmManualPIEComplete -MarkAllPIEItemsPassed"
}

Write-Host "[NEXT] Final validator:"
Write-Host "powershell -NoProfile -ExecutionPolicy Bypass -File Tools\Test-FWMVPPIEAcceptance.ps1"
exit 1
