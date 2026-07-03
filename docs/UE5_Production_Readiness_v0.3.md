# Unreal Engine 5 量產準備架構 v0.3

## 1. 文件目的

本文件是 `UE5_Architecture_Enhancement_v0.2.md` 之後的最後一層架構補強。

v0.1 定義遊戲與技術方向，v0.2 補上事件、資料管理、多人物件權威、Debug、狀態機等橫向系統。v0.3 的目標是讓專案具備長期量產、版本更新、測試、自動化、資料安全、調參與維護能力。

本文件完成後，架構設計應停止繼續抽象擴張，下一步應轉入 UE5 專案建立與 MVP 任務實作。

## 2. v0.3 新增系統總覽

```text
FutureWorld Production Readiness
├─ TestingAutomationSystem
├─ ConfigurationTuningSystem
├─ ModularGameFeatureBoundary
├─ ErrorHandlingFallbackSystem
├─ VersioningMigrationSystem
├─ AnalyticsTelemetrySystem
├─ AccessibilityLocalizationSystem
└─ SecurityAntiCheatPolicy
```

## 3. TestingAutomationSystem

### 3.1 為什麼需要

本遊戲包含玩家控制、武器、載具、AI、規則、重生、核心載具與未來多人連線。任何一個系統修改，都可能破壞完整遊戲循環。

測試系統的目的不是追求一開始就 100% 測試覆蓋，而是確保核心循環不會在迭代中悄悄壞掉。

### 3.2 測試分類

建議測試分成：

- Unit Tests：測試純邏輯，例如傷害計算、規則判斷、ContentId 解析。
- Functional Tests：測試 UE 關卡中的實際流程，例如進出載具、武器命中、重生。
- Automation Maps：專用測試地圖。
- Smoke Tests：每次大改後快速確認核心循環可用。
- Regression Tests：曾經壞過的功能必須留下測試。

### 3.3 MVP 必測流程

第一階段最重要的測試：

1. 玩家可以出生。
2. 玩家可以移動、跳躍、蹲下、爬行。
3. 玩家可以裝備武器並開火。
4. 武器命中 AI 後會造成傷害。
5. 武器命中載具後會造成傷害。
6. 玩家可以用 `F` 進入載具。
7. 玩家可以用 `F` 離開載具。
8. 載具生命歸零後會進入 Destroyed 狀態。
9. 核心載具被摧毀後扣一條命。
10. 玩家命數歸零後比賽結束。
11. AI 可以偵測玩家。
12. AI 可以攻擊玩家。

### 3.4 建議測試地圖

```text
Content/FutureWorld/Developer/TestMaps/
├─ L_Test_CombatGarage
├─ L_Test_WeaponDamage
├─ L_Test_VehicleEnterExit
├─ L_Test_CoreVehicleRule
├─ L_Test_AICombat
└─ L_Test_RespawnFlow
```

### 3.5 測試命名規則

```text
FW.Test.Player.Spawn
FW.Test.Player.EnterVehicle
FW.Test.Weapon.DamageCharacter
FW.Test.Weapon.DamageVehicle
FW.Test.Vehicle.DestroyedState
FW.Test.Rule.CoreVehicleLoseLife
FW.Test.AI.DetectPlayer
FW.Test.Match.EndWhenLivesZero
```

### 3.6 最小可用測試策略

MVP 前不需要建立龐大測試系統，但至少需要：

- 一張測試地圖 `L_Test_CombatGarage`
- 一份手動測試清單
- 一份 Smoke Test 清單
- 核心載具扣命流程測試
- 武器傷害測試
- 進出載具測試

## 4. ConfigurationTuningSystem

### 4.1 為什麼需要

遊戲好不好玩，很大一部分取決於數值調整。

如果數值散落在 C++、Blueprint、Data Asset、Widget 或關卡中，後期會非常難平衡。

### 4.2 調參資料分類

建議把調參資料分成：

- Weapon Balance
- Vehicle Balance
- Character Movement Balance
- AI Difficulty Balance
- Match Rule Balance
- Resource Spawn Balance
- Weather Gameplay Modifier

### 4.3 建議資料資產

```text
Content/FutureWorld/Data/Tuning/
├─ DA_Tuning_Global_Default
├─ DA_Tuning_Weapon_MVP
├─ DA_Tuning_Vehicle_MVP
├─ DA_Tuning_AI_Normal
├─ DA_Tuning_AI_Hard
├─ DA_Tuning_Rules_Competitive
└─ DA_Tuning_DebugSandbox
```

### 4.4 調參原則

- C++ 放預設安全值。
- Data Asset 放正式遊戲數值。
- Debug Profile 放測試數值。
- 不要在 Widget 裡寫平衡數值。
- 不要在關卡 Blueprint 裡寫武器傷害。
- 每次大幅調整數值時，記錄原因。

### 4.5 數值版本

每份重要平衡資料應有版本欄位：

```text
BalanceVersion: 0.1.0
Author: DesignerName
Purpose: MVP weapon baseline
LastReviewed: 2026-07-03
```

## 5. ModularGameFeatureBoundary

### 5.1 為什麼需要

未來你會新增：

- 新載具
- 新武器包
- 新地圖
- 天氣系統
- 線上多人
- 分割螢幕
- 新遊戲模式

如果一開始沒有模組邊界，後期會變成所有功能都塞在同一團。

### 5.2 建議模組邊界

短期仍可維持單一 UE module：`FutureWorld`。

但架構上應按以下邊界設計：

```text
FutureWorldCore
FutureWorldCharacters
FutureWorldVehicles
FutureWorldWeapons
FutureWorldCombat
FutureWorldAI
FutureWorldWorld
FutureWorldRules
FutureWorldWeather
FutureWorldOnline
FutureWorldUI
```

### 5.3 MVP 策略

MVP 不建議一開始拆太多 UE plugin，因為會增加建置與維護成本。

正確策略：

- Source 資料夾先按模組分清楚。
- 類別責任不要跨界。
- Data Asset 使用 `ContentId`。
- 未來需要時再拆成 plugin 或 Game Feature。

### 5.4 模組依賴方向

建議依賴方向：

```text
UI -> Core / Rules / Data
AI -> Characters / Weapons / Vehicles / Combat
Rules -> Core / Combat / Vehicles
Vehicles -> Combat / Events / Assets
Weapons -> Combat / Events / Assets
Characters -> Actions / Combat / Weapons
Core -> Assets / Events / Save
```

避免：

- Weapon 依賴 UI
- Vehicle 依賴 Widget
- AI 直接依賴 MainMenu
- Weather 直接修改 Weapon Blueprint

## 6. ErrorHandlingFallbackSystem

### 6.1 為什麼需要

資料驅動遊戲一定會遇到資料缺失、ID 錯誤、存檔版本不符、資產載入失敗。

如果沒有 Fallback 策略，錯一筆資料就可能讓整場比賽不能開始。

### 6.2 必備 Fallback 規則

武器：

- 武器 ID 找不到時，使用 `Weapon.Pistol.Basic`。
- 武器模組找不到時，忽略該模組並記錄警告。
- 彈藥資料缺失時，使用安全預設值。

載具：

- 載具 ID 找不到時，使用 `Vehicle.Car.Balanced` 或取消生成。
- 核心載具生成失敗時，通知 RuleSystem 並重試。
- 載具座位資料缺失時，至少保留駕駛座。

角色：

- 角色外觀缺失時，使用預設角色 Mesh。
- 動畫組缺失時，使用預設動畫組。

規則：

- RuleSet 缺失時，使用 `RuleSet.Competitive.Default`。
- 難度缺失時，使用 `Difficulty.Normal`。

存檔：

- Loadout 讀取失敗時，回復預設 Loadout。
- 舊版本存檔讀取時，先跑 migration。
- 無法 migration 時，保留設定，重置不相容進度。

### 6.3 錯誤層級

建議分級：

- Info：正常流程紀錄。
- Warning：資料缺失但已 fallback。
- Error：功能失敗但遊戲仍可繼續。
- Fatal：無法繼續遊戲，需要返回主選單或中止。

### 6.4 最小實作

MVP 至少需要：

- `FWLog` 分類
- 資料查找失敗時的 Warning
- 預設武器 fallback
- 預設載具 fallback
- 預設 RuleSet fallback

## 7. VersioningMigrationSystem

### 7.1 為什麼需要

遊戲會持續演進，早期資料一定會改。

會改的內容包括：

- SaveGame 結構
- Loadout 結構
- ContentId 命名
- WeaponData 欄位
- VehicleData 欄位
- RuleSet 欄位

沒有遷移策略，早期存檔或資料可能在幾週後就讀不了。

### 7.2 版本類型

建議定義：

- SaveVersion
- DataSchemaVersion
- BalanceVersion
- ContentVersion
- BuildVersion

### 7.3 ContentId 遷移

範例：

```text
Weapon.Rifle.Default -> Weapon.Rifle.Basic
Vehicle.Bike.Fast -> Vehicle.Motorcycle.Fast
Rule.Default -> RuleSet.Competitive.Default
```

### 7.4 Save Migration 原則

讀取存檔時：

1. 檢查 SaveVersion。
2. 如果版本相同，直接讀取。
3. 如果版本較舊，依序執行 migration。
4. 如果某個 ContentId 已廢棄，查表轉換。
5. 如果無法轉換，使用 fallback。
6. 儲存成最新版本。

### 7.5 MVP 最小需求

MVP 至少要在存檔資料中保留：

```text
SaveVersion
BuildVersion
CreatedAt
LastSavedAt
```

## 8. AnalyticsTelemetrySystem

### 8.1 為什麼需要

戰術遊戲的平衡不能只靠感覺。你需要知道：

- 哪把武器太強
- 哪台載具太弱
- AI 是否太準
- 玩家死亡原因是否集中
- 核心載具規則是否有趣
- 每場比賽是否太短或太長

MVP 不需要接雲端，但可以先做本機 Telemetry Log。

### 8.2 建議事件

```text
Telemetry.Match.Start
Telemetry.Match.End
Telemetry.Player.Death
Telemetry.Player.Respawn
Telemetry.Weapon.Fired
Telemetry.Weapon.Hit
Telemetry.Vehicle.Enter
Telemetry.Vehicle.Destroyed
Telemetry.CoreVehicle.Destroyed
Telemetry.AI.KillPlayer
Telemetry.Rule.LifeLost
```

### 8.3 建議記錄欄位

每筆事件可包含：

- EventName
- Timestamp
- MatchId
- PlayerId
- InstigatorContentId
- TargetContentId
- WeaponContentId
- VehicleContentId
- DamageType
- WorldLocation
- DifficultyId
- RuleSetId

### 8.4 使用原則

- Telemetry 不應影響遊戲結果。
- Telemetry 寫入失敗不應中斷遊戲。
- 線上版本需注意隱私與法規。
- MVP 可先輸出到本機 `.log` 或 `.csv`。

## 9. AccessibilityLocalizationSystem

### 9.1 為什麼需要

無障礙與在地化如果太晚做，會造成大量 UI 與文字重工。

本遊戲一開始就應避免硬寫不可翻譯字串。

### 9.2 Localization 原則

- UI 顯示文字使用 `FText`。
- 不在 Blueprint 中散落硬寫長句。
- 武器、載具、角色名稱來自 Data Asset。
- Match Rule 說明可翻譯。
- Debug 字串可例外，但正式 UI 不可硬寫。

### 9.3 Accessibility 預留

建議預留：

- 字體大小調整
- UI 縮放
- 色盲模式
- 字幕
- 音效提示替代文字
- 按鍵重綁
- 控制器震動開關
- 瞄準輔助強度
- Camera shake 開關
- Motion blur 開關

### 9.4 MVP 最小需求

MVP 至少要：

- UI 使用可在地化文字
- 支援按鍵設定資料化
- 不把重要資訊只用顏色表示
- 提供字幕系統預留點

## 10. SecurityAntiCheatPolicy

### 10.1 為什麼需要

MVP 是單機，但未來有線上多人。現在不需要做完整反作弊，但架構上要避免明顯漏洞。

### 10.2 基礎安全規則

Client 不可直接決定：

- 傷害結果
- 是否命中
- 是否扣命
- 是否摧毀載具
- 是否取得武器
- 是否解鎖內容
- 是否改變比賽規則

Server 必須驗證：

- 開火頻率
- 彈藥數量
- 武器是否已裝備
- 互動距離
- 載具進入是否合法
- 玩家狀態是否允許該動作
- 傷害來源是否合法

### 10.3 存檔與線上資料

原則：

- 本機存檔不能作為線上權威資料。
- 線上模式下，解鎖、貨幣、排名需由伺服器或可信服務保存。
- 本機 Loadout 進入線上前需由 Server 驗證。

### 10.4 MVP 最小需求

MVP 先在命名與流程上預留：

- `Server_RequestFire`
- `Server_RequestEnterVehicle`
- `Server_RequestRespawn`
- `Server_ApplyDamage`
- `Server_SetLoadout`

即使初期未真正連線，也要讓架構語意正確。

## 11. 文件與任務管理規則

### 11.1 文件版本規則

建議：

- v0.1：設計與核心架構
- v0.2：架構補強
- v0.3：量產準備
- v1.0：MVP 實作完成後更新為正式規格

### 11.2 文件更新原則

- 設計變更要更新對應文件。
- 類別名稱變更要更新骨架文件。
- 規則變更要更新 GDD 與任務文件。
- 已實作內容與文件矛盾時，要明確標註並修正。

### 11.3 任務管理建議

下一步應建立：

- `UE5_MVP_Implementation_Tasks_v0.2.md`

每個任務應包含：

- 任務目標
- 相關系統
- 需要建立的類別或資產
- 驗收條件
- 測試方式
- 風險備註

## 12. v0.3 完成標準

當以下規格清楚定義後，v0.3 即完成：

- 核心玩法有手動與自動化測試方向。
- 數值調整有 Data Asset 與版本規則。
- 模組邊界清楚，未來可拆 plugin。
- 資料缺失時有 fallback 策略。
- SaveGame、ContentId、Balance 有版本遷移方向。
- Telemetry 可以記錄核心戰鬥與比賽事件。
- UI 與資料文字預留在地化。
- 多人模式的基礎安全規則已定義。

## 13. 最終架構判斷

完成 v0.3 後，架構完整度可評估為：

```text
原型開發：98% 完整
MVP 開發：95% 完整
未來多人：85% 完整
長期量產：88% 完整
```

剩下的完整度不應再靠文件推進，而應透過實作、測試與迭代補足。

下一步不要再繼續抽象設計，應開始建立：

- UE5 C++ 專案
- MVP 任務清單
- 第一張測試地圖
- 第一個玩家、武器、載具、AI 原型

