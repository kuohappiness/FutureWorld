# FW MVP Manual Test Checklist v0.1

## Purpose

This checklist keeps MVP verification repeatable. Run it in `L_Test_CombatGarage` after major gameplay changes.
Create a final manual PIE evidence session with `Tools\New-FWMVPPIEAcceptanceSession.ps1`, use `Tools\Get-FWMVPPIEAcceptanceStatus.ps1` to confirm the active evidence folder and next command, open the test map and auto-start PIE with `Tools\Open-FWMVPCombatGaragePIE.ps1 -StartPIE`, capture evidence with `Tools\Invoke-FWMVPPIEPlayEvidenceCapture.ps1 -SkipInput`, export runtime log proof with `Tools\Export-FWMVPPIERuntimeEvidence.ps1`, record each manually observed checklist item with `Tools\Set-FWMVPPIEChecklistItem.ps1`, record movement input evidence with `Tools\Invoke-FWMVPPIEMovementEvidenceCapture.ps1` when verifying walk/run/jump/crouch/crawl, record camera toggle evidence with `Tools\Invoke-FWMVPPIECameraEvidenceCapture.ps1`, record car and motorcycle enter/exit evidence with `Tools\Open-FWMVPCombatGaragePIE.ps1 -StartPIE -VehicleInteractionEvidence -VehicleInteractionTarget Car` or `-VehicleInteractionTarget Motorcycle` and, when a visible viewport is available, `Tools\Invoke-FWMVPPIEVehicleInteractionEvidenceCapture.ps1`, record vehicle driving evidence with `Tools\Open-FWMVPCombatGaragePIE.ps1 -StartPIE -VehicleDriveEvidence -VehicleDriveTarget Car` or `-VehicleDriveTarget Motorcycle`, record additional screenshots or video in the generated evidence folder or capture PNGs with `Tools\Capture-FWMVPPIEEvidenceScreenshot.ps1`, finalize `docs\FW_MVP_PIE_Acceptance_Report.md` with `Tools\Complete-FWMVPPIEAcceptanceReport.ps1`, then run `Tools\Test-FWMVPPIEAcceptance.ps1` before treating MVP as manually accepted.

## Preflight

- Project opens in Unreal Editor.
- `Tools\Run-FWMVPPIEPreflight.ps1 -ProjectFile D:\Projects\FutureWorld\FW.uproject` completes successfully before final manual PIE; use `-ListOnly` to inspect the ordered commands without running heavy UE gates.
- `FWEditor` builds successfully.
- `Tools\Run-FWMVPAssetBootstrap.ps1 -TimeoutSeconds 900` completes successfully.
- `Tools\Run-FWMVPVerify.ps1 -TimeoutSeconds 900` completes successfully.
- `Tools\Run-FWMVPRuntimeVerify.ps1 -TimeoutSeconds 900` completes successfully and confirms player input semantic handlers, AI combat states, weapon damage against AI and vehicles, car/motorcycle driving semantics, safe-start AI spacing that leaves the player alive at 100/100, and the actual `L_Test_CombatGarage` fixture can drive runtime setup in an isolated world.
- `Tools\Run-FWMVPFullLoopVerify.ps1 -TimeoutSeconds 900` completes successfully and confirms the canonical `L_Test_CombatGarage` fixture can run a headless full-loop simulation through director setup, match start, player weapon damage, director-spawned AI attack, vehicle enter/drive/exit, HUD snapshots, life penalty, respawn continuation, and zero-lives match end.
- `Tools\Run-FWMVPSmokeTest.ps1 -TimeoutSeconds 900` completes successfully and confirms the UE Automation smoke test discovers and verifies MVP DataAssets plus the canonical test-map fixture.
- `Tools\Test-FWMVPReadiness.ps1 -StrictAssets -VerifyPIEValidator` completes successfully and verifies source/docs/assets plus the PIE preflight list self-test, preflight/session gate-list consistency, PIE acceptance validator positive and negative paths, and the validator synthetic cleanup self-test.
- `Tools\New-FWMVPPIEAcceptanceSession.ps1` creates the final evidence folder, writes `README.md` and initial `manifest.json` before gates run, archives updated automated gate logs before manual PIE, captures each gate's `OutputLog`, records the `PIELaunchDryRun` gate, and preserves failed gate status/output if a preflight step fails.
- `Tools\Open-FWMVPCombatGaragePIE.ps1 -DryRun` verifies the project, editor, and canonical `L_Test_CombatGarage` map path; running it with `-StartPIE` opens Unreal Editor on the correct manual PIE map and asks UE to auto-start PIE with the editor `-pie` flag. Live Coding is disabled by default to avoid background `UnrealBuildTool.exe` popups during acceptance capture. Use `-EnableLiveCoding` only when deliberately testing code iteration.
- `Tools\Capture-FWMVPPIEEvidenceScreenshot.ps1` captures a PNG screenshot into the active evidence folder after manual PIE is visible, validates the PNG signature and minimum evidence size, refuses output paths outside the evidence folder, and supports `-WindowOnly` capture that uses the Unreal Editor window with `PrintWindow` to avoid Codex/console overlap; `Tools\Test-FWMVPPIEEvidenceCapture.ps1` verifies its dry-run and path-safety behavior without requiring a live desktop capture.
- `Tools\Invoke-FWMVPPIEPlayEvidenceCapture.ps1 -SkipInput` brings the running Unreal Editor window to the foreground, waits for UE log proof that PIE started, and captures a PNG evidence screenshot without toolbar clicking; without `-SkipInput`, it can still fall back to mouse/keyboard Play input. `Tools\Test-FWMVPPIEPlayEvidenceCapture.ps1` verifies dry-run and invalid-folder behavior without sending input.
- `Tools\Invoke-FWMVPPIEMovementEvidenceCapture.ps1` brings the PIE viewport forward, sends deterministic WASD/LeftShift/Space/Ctrl/C input, and captures HUD movement debug screenshots. The HUD movement debug line exposes location, speed/max speed, falling/crouched/crawling flags, and the latest movement input handler so slow screenshots can still prove input response.
- `Tools\Invoke-FWMVPPIECameraEvidenceCapture.ps1` captures third-person, first-person, and restored third-person screenshots after toggling `V`; the HUD camera line exposes `Camera: ThirdPerson` or `Camera: FirstPerson` so the visual evidence proves the toggle state as well as framing.
- `Tools\Open-FWMVPCombatGaragePIE.ps1 -StartPIE -VehicleInteractionEvidence -VehicleInteractionTarget Car|Motorcycle` writes a `*_state-evidence.jsonl` file for `PIE-09`/`PIE-10` with `Enter Vehicle`, possessed target vehicle, `Exit Vehicle`, vehicle health, and returned on-foot state. `Tools\Invoke-FWMVPPIEVehicleInteractionEvidenceCapture.ps1` can add synchronized screenshots when the local editor viewport is visible.
- `Tools\Open-FWMVPCombatGaragePIE.ps1 -StartPIE -VehicleDriveEvidence -VehicleDriveTarget Car|Motorcycle` writes a `*_state-evidence.jsonl` file for vehicle driving checks with throttle, reverse/brake, steering yaw, camera control rotation, collision setup, and a blocking map collision probe.
- `F9` in PIE fires the current player weapon at the nearest living AI through the normal `AFWWeaponBase::FireAtActor` path for acceptance evidence. The HUD AI debug block exposes AI count, nearest AI health/state, and the last combat debug result so `PIE-07` can prove AI damage without relying on blind aiming in a dark placeholder map.
- `F10` in PIE fires the current player weapon at the nearest active vehicle through the normal `AFWWeaponBase::FireAtActor` path for acceptance evidence. The HUD vehicle debug block exposes vehicle count, nearest vehicle health/state, and the last combat debug result so `PIE-08` can prove vehicle damage without relying on blind aiming in a dark placeholder map.
- `Tools\Export-FWMVPPIERuntimeEvidence.ps1` extracts `FW.log` lines proving PIE was started for `L_Test_CombatGarage`, including `UEDPIE_0_L_Test_CombatGarage`, `FWCompetitiveGameMode`, and `Bringing World ... up for play`; `Tools\Test-FWMVPPIERuntimeEvidence.ps1` verifies positive and negative synthetic log paths.
- `Tools\Set-FWMVPPIEChecklistItem.ps1` records one manually observed `PIE-00` through `PIE-23` checklist item at a time, checks that item in the report, and writes or updates `PIE-checklist-evidence.json` with timestamp, tester, evidence note, and the report line; `Tools\Test-FWMVPPIEChecklistLedger.ps1` verifies upsert behavior and rejects invalid item IDs or empty notes.
- `Tools\Test-FWMVPPIEAcceptanceValidator.ps1` verifies the PIE acceptance validator itself with isolated synthetic evidence by checking that complete evidence passes, tiny media evidence fails, invalid media header evidence fails, missing `OutputLog` evidence fails, missing README/ProjectFile metadata fails, and a failed session gate still writes README, manifest, and output evidence; automatic synthetic output is cleaned up after success unless `-KeepOutput` or a custom `-OutputRoot` is used.
- `Tools\Complete-FWMVPPIEAcceptanceReport.ps1` finalizes the manual PIE report after real PIE has been completed; `Pass` and all-item checkoff require `-ConfirmManualPIEComplete`, the target evidence folder must already contain at least one candidate screenshot/video file, and a temporary preview report must pass `Tools\Test-FWMVPPIEAcceptance.ps1` before the real report is written.
- `Tools\Test-FWMVPPIEAcceptanceFinalizer.ps1` verifies the finalizer's positive path and safety rails with isolated synthetic evidence, including rejection of `Pass` without explicit manual confirmation, rejection of `Pass` when no candidate media file exists, and rejection of invalid media headers without mutating the real report.
- `Tools\Get-FWMVPPIEAcceptanceStatus.ps1` summarizes the current report, evidence folder, checklist count, candidate media count, manifest gate count, and prints the next exact command for the current state; `Tools\Test-FWMVPPIEAcceptanceStatus.ps1` verifies incomplete, complete, and missing-media status paths with isolated synthetic evidence.
- `Tools\Test-FWMVPPIEPreflightList.ps1` verifies the preflight runner's `-ListOnly` output includes all required gates in order, with `Readiness` using `-VerifyPIEValidator` and `PIELaunchDryRun` using `-DryRun`.
- `Tools\Test-FWMVPPIESessionGateList.ps1` verifies `Tools\Run-FWMVPPIEPreflight.ps1 -ListOnly` and `Tools\New-FWMVPPIEAcceptanceSession.ps1 -ListPreflightGates` expose the same preflight gate order.
- `Tools\Clear-FWMVPPIEValidatorSynthetic.ps1` can dry-run or clean old `Saved\PIEAcceptanceValidatorSynthetic\Run-*` folders, skips active `Run-<PID>` folders, and only affects validator synthetic self-test output, not final manual PIE evidence folders.
- `Tools\Test-FWMVPPIEValidatorSyntheticCleanup.ps1` verifies the cleanup tool with an isolated synthetic root, confirming dry-run does not delete and `-Apply` deletes only `Run-*` folders.
- `Tools\Test-FWMVPPIEAcceptance.ps1` remains incomplete until the final manual PIE report is filled, the evidence folder contains non-empty `README.md`, `manifest.json`, `PIE-runtime-evidence.log`, and `PIE-checklist-evidence.json`, the runtime evidence proves PIE startup for `L_Test_CombatGarage` with `FWCompetitiveGameMode`, the checklist ledger contains all 24 `PIE-00` through `PIE-23` records with timestamp, tester, evidence note, and report line, the manifest `CreatedAt`, `Tester`, and `.uproject` `ProjectFile` are present, the manifest `SessionDir` matches the report evidence folder, all preflight and `PIELaunchDryRun` gates in the manifest passed with non-empty `OutputLog` files, and the folder contains at least one valid PNG/JPEG/MP4/MOV/WebM screenshot or video file >= 1024 bytes; when it passes, it prints the accepted session metadata, media evidence with SHA256 hashes, runtime evidence hash, and manifest gates for review; final commit/push requires it to pass, or `Tools\Test-FWMVPReadiness.ps1 -StrictAssets -VerifyPIEValidator -RequirePIEAcceptance` to pass.
- Test map uses `FWCompetitiveGameMode`.
- `AFWCombatGarageTestDirector` is present.
- Player, AI, vehicle, weapon, and director classes are assigned by C++ defaults, generated assets, or verified map fixtures.
- The generated map fixture contains one player spawn, three AI spawns, one car spawn, one motorcycle spawn, and one weapon spawn after bootstrap normalization.
- `AFWCombatGarageTestDirector` enforces a minimum initial AI distance from the player during setup, so close or stale AI spawn points cannot immediately put the player into `State.Character.Dead` before manual PIE observation starts.

## Player

- Player spawns at the player spawn point; commandlet coverage already verifies the actual map fixture moves the runtime player to the player spawn, so PIE should focus on camera, collision, and playable framing.
- Player can walk, run, jump, crouch, and crawl; commandlet coverage already verifies move/run/crawl/dead-movement semantics, and PIE evidence should confirm the HUD movement debug line changes for `Move`, run max speed, `JumpStart`/`JumpStop`, `CrouchOn`, and `CrawlOn` while the viewport remains controllable.
- Death state prevents movement.
- Camera toggles between third-person and first-person; commandlet coverage already verifies the camera mode flag, and PIE evidence should show `Camera: ThirdPerson`, `Camera: FirstPerson`, and restored `Camera: ThirdPerson` with usable visual framing.
- `E` detects and calls interaction on a nearby interactable.

## Weapons

- Player can equip the pistol, rifle, and sniper MVP weapons.
- `1`, `2`, and `3` switch to pistol, rifle, and sniper; commandlet coverage already verifies slot handler ContentIds.
- HUD weapon ContentId changes after switching weapon slots.
- Ammo decreases when firing; commandlet coverage already verifies input-driven fire consumes ammo.
- Empty magazine prevents further fire.
- Reload restores magazine ammo.
- Hitscan damage applies through `UFWHealthComponent::ApplyDamageRequest`.
- Weapon fire broadcasts `Event.Weapon.Fired`.
- Damage broadcasts `Event.Damage.Applied`.

## Vehicles

- Player can enter and exit a car with `E`; commandlet coverage already verifies interaction trace into a visible vehicle enters and exits, and `PIE-09` evidence should show or record `Interact: Enter Vehicle`, then possessed `FWCarVehicle` with `Vehicle: 650/650` and `Interact: Exit Vehicle`, then a return to on-foot HUD state after pressing `E` again.
- Player can enter and exit a motorcycle with `E`; `PIE-10` evidence should show or record `Interact: Enter Vehicle`, then possessed `FWMotorcycleVehicle` with `Vehicle: 300/300` and `Interact: Exit Vehicle`, then a return to on-foot HUD state after pressing `E` again.
- Car driving is usable; `PIE-11` evidence should show or record positive forward speed/distance from throttle, negative forward speed from reverse/brake, steering yaw change, camera control rotation response, and blocking map collision presentation.
- Motorcycle driving is usable; `PIE-12` evidence should show or record positive forward speed/distance from throttle, negative forward speed from reverse/brake, steering yaw change, camera control rotation response, and a blocking collision presentation.
- Player can enter and exit a motorcycle with `E`.
- Car responds to `W` throttle, `S` reverse/brake, and `A`/`D` steering; commandlet coverage already verifies possession, forward throttle movement, steering yaw, look-handler stability, and exit input restore, so PIE should focus on feel, reverse/brake, collision, and camera presentation.
- Motorcycle responds to `W` throttle, `S` reverse/brake, and `A`/`D` steering; commandlet coverage already verifies possession, forward throttle movement, steering yaw, look-handler stability, and exit input restore, so PIE should focus on feel, reverse/brake, collision, and camera presentation.
- Mouse look controls the possessed vehicle camera; commandlet coverage only verifies the handler is stable without a local-player viewport, so PIE remains the authoritative visual check.
- Vehicle health decreases when shot; commandlet coverage already verifies pistol damage reduces both car and motorcycle health. For `PIE-08`, focus the PIE viewport, capture the HUD before pressing `F10`, press `F10`, then capture the HUD showing nearest vehicle health reduced and `Combat: F10 nearest vehicle fired=Yes`.
- Vehicle enters `State.Vehicle.Destroyed` when health reaches zero.
- Vehicle destroyed event fires once.
- First player core vehicle is registered in `AFWPlayerState`; commandlet coverage already verifies this from the actual map fixture, so PIE should confirm the HUD/rules presentation.

## AI

- At least three AI spawn in the test map.
- Default AI safe-start spacing leaves the player alive and out of `State.Character.Dead` before combat is deliberately engaged; commandlet coverage verifies the actual map fixture with default AI controllers ticked.
- AI detects the player within detection radius; commandlet coverage already verifies detected/out-of-range AI enters `State.AI.Alert`.
- AI pursues when outside attack range; PIE should focus on pathing and movement presentation.
- AI attacks when inside attack range; commandlet coverage already verifies `State.AI.AttackOnFoot`, including a director-spawned AI in the canonical fixture during headless full-loop verification.
- AI can damage the player; commandlet coverage already verifies runtime AI weapon fire reduces player health, including full-loop fixture AI attack.
- AI can be damaged and killed; for `PIE-07`, focus the PIE viewport, capture the HUD before pressing `F9`, press `F9`, then capture the HUD showing nearest AI health reduced and `Combat: F9 nearest AI fired=Yes`.
- AI state appears in Debug output; commandlet coverage already verifies AI state transitions, while PIE should confirm on-screen Debug presentation.

## Rules And Respawn

- Match enters `Match.InProgress` after test setup.
- Core vehicle destruction subtracts the configured life penalty.
- Player respawns if lives remain and respawn is enabled.
- Lives reaching zero ends the match.
- `Event.Respawn.Completed` fires after respawn.

## HUD And Debug

- HUD displays health, lives, current weapon, ammo, and core vehicle health.
- HUD visual values match the state snapshots covered by `FWMVPRuntimeVerify`.
- Debug event log shows recent gameplay events.
- Debug actor state shows health, state, and hit zone data.
- Debug visual output matches the event buffer covered by `FWMVPRuntimeVerify`.
- No UI action directly changes health, lives, vehicle destruction, or match end state.

## Performance Smoke

- Test map remains responsive with the default AI count.
- No obvious per-frame log spam.
- No unbounded event log growth.
- No unexpected Tick usage outside intentional systems.
- Headless full-loop verification passes before manual PIE, so visual testing starts from a known working rules/combat/AI/vehicle state path.
