# Unreal Engine 5 MVP 實作任務清單 v0.2

## 1. 文件目的

本文件將目前的遊戲設計與技術架構拆成可執行的 UE5 MVP 任務。

嚴謹標準：

- 每個階段都有明確目標。
- 每個任務都有產出物。
- 每個任務都有驗收條件。
- 每個階段都有 Gate，未通過不得進入下一階段。
- 優先打通核心循環，再擴大內容量。
- 不先做完整大地圖，不先堆大量武器，不先追求美術完成度。

## 2. MVP 成功定義

MVP 不是完整遊戲，而是證明核心玩法可行的第一個可玩版本。

MVP 完成時，必須達成：

- 玩家可在測試地圖出生。
- 玩家可走、跑、跳、蹲、爬行。
- 玩家可切換第三人稱與第一人稱視角。
- 玩家可裝備手槍、步槍、狙擊槍。
- 玩家可射擊 AI 與載具。
- 玩家可用 `F` 進入與離開車子、機車。
- 載具具備生命值與毀損狀態。
- 核心載具被摧毀時，擁有者扣一條命。
- 玩家可重生。
- 多個 AI 可巡邏、偵測、追擊、攻擊玩家。
- 至少一種 AI 可嘗試使用載具或追逐載具目標。
- 比賽可開始、進行、結束。
- Debug HUD 可顯示玩家、武器、載具、AI、規則狀態。
- 所有核心內容使用 `ContentId`。

## 3. 優先級定義

```text
P0 = 沒有它，MVP 不成立
P1 = MVP 需要，但可在核心循環打通後完成
P2 = MVP 加分或開發效率提升
P3 = MVP 後再做
```

## 4. 階段總覽

```text
Phase 0   專案建立與基礎設定
Phase 0.5 橫向底層系統
Phase 1   玩家控制與攝影機
Phase 2   戰鬥與武器
Phase 3   載具與互動
Phase 4   規則、命數、重生
Phase 5   AI 原型
Phase 6   測試地圖與玩法整合
Phase 7   UI、設定、Loadout
Phase 8   Debug、測試、效能與穩定化
Phase 9   MVP 驗收
```

## 5. Phase 0：專案建立與基礎設定

### 5.1 目標

建立乾淨、可編譯、可長期擴充的 UE5 C++ 專案。

### 5.2 任務

#### FW-MVP-0001：建立 UE5 C++ 專案

優先級：P0

產出：

- UE5 C++ 專案 `FW`
- 初始 `.uproject`
- `Source/FW`
- 可開啟的 Editor 專案

驗收條件：

- 專案可在 Unreal Editor 開啟。
- 專案可成功編譯。
- 預設地圖可 Play In Editor。

測試方式：

- 開啟 Editor。
- 執行 Compile。
- 進入 PIE。

#### FW-MVP-0002：建立 Source 資料夾結構

優先級：P0

產出：

- `Core`
- `Characters`
- `Actions`
- `Weapons`
- `Vehicles`
- `Combat`
- `AI`
- `World`
- `Rules`
- `Events`
- `Assets`
- `State`
- `Debug`
- `UI`

驗收條件：

- 資料夾命名與架構文件一致。
- 新類別可放入對應資料夾並編譯。

#### FW-MVP-0003：建立 Content 資料夾結構

優先級：P0

產出：

- `Content/FW/Characters`
- `Content/FW/Vehicles`
- `Content/FW/Weapons`
- `Content/FW/AI`
- `Content/FW/Maps`
- `Content/FW/UI`
- `Content/FW/Input`
- `Content/FW/Data`
- `Content/FW/Developer`

驗收條件：

- 資料夾結構清楚。
- 測試資產放在 `Developer` 或 `TestMaps`，不混入正式內容。

#### FW-MVP-0004：設定 Build.cs 基礎依賴

優先級：P0

產出：

- `EnhancedInput`
- `UMG`
- `AIModule`
- `GameplayTasks`
- `NavigationSystem`

驗收條件：

- 專案編譯成功。
- 可引用 Enhanced Input 與 AI 類別。

### 5.3 Phase 0 Gate

必須達成：

- 專案可開啟。
- 專案可編譯。
- Source / Content 結構完成。
- 基礎依賴可用。

未通過不得進入 Phase 1。

## 6. Phase 0.5：橫向底層系統

### 6.1 目標

先建立最小可用的事件、資料 ID、狀態機、Debug 與傷害入口，避免後續系統互相硬接。

### 6.2 任務

#### FW-MVP-0050：建立 `UFWPrimaryDataAsset`

優先級：P0

產出：

- `UFWPrimaryDataAsset`
- `ContentId`
- `DisplayName`
- `Description`
- `ContentTags`

驗收條件：

- 武器、載具、規則、難度 Data Asset 可繼承此類別。
- 每個主要資料資產都有穩定 `ContentId`。

#### FW-MVP-0051：建立 `FFWGameplayEvent`

優先級：P0

產出：

- `FFWGameplayEvent`
- `EventTag`
- `Instigator`
- `Target`
- `Magnitude`
- `WorldLocation`

驗收條件：

- C++ 與 Blueprint 都能建立事件資料。
- 可用 Gameplay Tag 區分事件類型。

#### FW-MVP-0052：建立 `UFWEventSubsystem` 最小版

優先級：P0

產出：

- 廣播事件函式
- Blueprint 可監聽事件
- Debug 可印出事件名稱

驗收條件：

- 武器開火可廣播 `WeaponFired`。
- 載具毀損可廣播 `VehicleDestroyed`。
- 規則系統可監聽核心載具事件。

#### FW-MVP-0053：建立 `UFWStateMachineComponent` 最小版

優先級：P0

產出：

- CurrentState
- PreviousState
- ChangeState
- OnStateChanged

驗收條件：

- 玩家可切換 `OnFoot`、`InVehicle`、`Dead`。
- 載具可切換 `Idle`、`Occupied`、`Destroyed`。
- Debug HUD 可讀取目前狀態。

#### FW-MVP-0054：建立 `UFWHealthComponent`

優先級：P0

產出：

- MaxHealth
- CurrentHealth
- ApplyDamage
- OnHealthChanged
- OnDeath

驗收條件：

- 角色、AI、載具都可使用。
- 生命歸零只觸發一次死亡事件。
- 傷害入口集中，不散落在 Blueprint。

#### FW-MVP-0055：建立 Debug 開關基礎

優先級：P1

產出：

- `FWDebugSubsystem` 或 Debug HUD 佔位
- 顯示玩家狀態
- 顯示目前武器
- 顯示載具生命
- 顯示 AI 狀態

驗收條件：

- PIE 中可開關 Debug 顯示。
- 不影響正式玩法流程。

### 6.3 Phase 0.5 Gate

必須達成：

- Data Asset 有 ContentId 基底。
- EventSystem 可廣播與監聽。
- StateMachine 可用。
- HealthComponent 可用。
- Debug 至少可顯示玩家狀態。

未通過不得開始武器與載具大量實作。

## 7. Phase 1：玩家控制與攝影機

### 7.1 目標

建立可玩的玩家角色，完成基本移動、攝影機與輸入 Context。

### 7.2 任務

#### FW-MVP-0100：建立 `AFWPlayerCharacter`

優先級：P0

產出：

- 玩家角色 C++ 類別
- Blueprint 子類 `BP_PlayerCharacter`
- HealthComponent
- StateMachineComponent
- CameraComponent / SpringArm

驗收條件：

- 玩家可在測試地圖出生。
- 玩家狀態預設為 `OnFoot`。

#### FW-MVP-0101：設定 Enhanced Input

優先級：P0

產出：

- `IA_Move`
- `IA_Look`
- `IA_Jump`
- `IA_Run`
- `IA_Crouch`
- `IA_Crawl`
- `IA_Fire`
- `IA_Aim`
- `IA_Reload`
- `IA_Interact`
- `IA_ToggleCamera`
- `IMC_OnFoot`
- `IMC_Vehicle`
- `IMC_Menu`

驗收條件：

- 步行狀態使用 `IMC_OnFoot`。
- 進入載具後可切到 `IMC_Vehicle`。

#### FW-MVP-0102：實作移動能力

優先級：P0

產出：

- 走路
- 跑步
- 跳躍
- 蹲下
- 爬行

驗收條件：

- 每個動作可用鍵盤觸發。
- 動作之間不產生互斥錯誤。
- 死亡狀態不可移動。

#### FW-MVP-0103：實作攝影機切換

優先級：P0

產出：

- 第三人稱視角
- 第一人稱視角
- `V` 鍵切換

驗收條件：

- 切換視角不破壞瞄準方向。
- 進入載具後使用載具攝影機。

#### FW-MVP-0104：建立互動掃描

優先級：P0

產出：

- `IFWInteractable`
- 玩家互動 Trace
- 最近可互動目標

驗收條件：

- 玩家靠近可互動物件時能偵測。
- `F` 可呼叫 Interact。
- Debug 顯示目前互動目標。

### 7.3 Phase 1 Gate

必須達成：

- 玩家控制手感可接受。
- 五種移動能力可用。
- 攝影機可切換。
- `F` 互動框架可用。

## 8. Phase 2：戰鬥與武器

### 8.1 目標

建立可擴充武器系統，先完成手槍、步槍、狙擊槍與基礎傷害流程。

### 8.2 任務

#### FW-MVP-0200：建立 `UFWWeaponData`

優先級：P0

產出：

- WeaponId / ContentId
- WeaponType
- BaseDamage
- FireRate
- MagazineSize
- ReloadTime
- Range
- VehicleDamageMultiplier
- WeakPointMultiplier

驗收條件：

- 三把 MVP 武器都使用 Data Asset。
- 不在武器 Blueprint 中硬寫主要數值。

#### FW-MVP-0201：建立 `AFWWeaponBase`

優先級：P0

產出：

- Equip
- Unequip
- StartFire
- StopFire
- Reload
- Ammo 狀態

驗收條件：

- 玩家可裝備武器。
- 武器可開火與換彈。
- 彈藥耗盡不可繼續開火。

#### FW-MVP-0202：建立三把 MVP 武器

優先級：P0

產出：

- `BP_Weapon_Pistol`
- `BP_Weapon_Rifle`
- `BP_Weapon_Sniper`
- 對應 Data Asset

驗收條件：

- 三把武器射速、傷害、彈匣不同。
- 狙擊槍對弱點倍率較高。

#### FW-MVP-0203：實作命中與傷害

優先級：P0

產出：

- Hitscan 或 Projectile 初版
- Damage Request
- Hit Result
- DamageApplied 事件

驗收條件：

- 武器可傷害 AI。
- 武器可傷害載具。
- 命中後觸發事件。
- Debug 可顯示命中與傷害數值。

#### FW-MVP-0204：建立 `UFWHitZoneComponent`

優先級：P1

產出：

- HitZoneName
- DamageMultiplier
- Character Hit Zones
- Vehicle Hit Zones

驗收條件：

- 擊中弱點時傷害倍率正確。
- 車輛弱點可被 Debug 顯示。

### 8.3 Phase 2 Gate

必須達成：

- 三把武器可用。
- 武器可造成傷害。
- 傷害走集中流程。
- 武器事件與 Debug 可用。

## 9. Phase 3：載具與互動

### 9.1 目標

完成車子與機車的最小可玩版本，支援 `F` 上下車、生命值、毀損與核心載具歸屬。

### 9.2 任務

#### FW-MVP-0300：建立 `UFWVehicleData`

優先級：P0

產出：

- VehicleId / ContentId
- MaxHealth
- Armor
- MaxSpeed
- Acceleration
- Handling
- SeatCount

驗收條件：

- 車子與機車使用 Data Asset。
- 載具主要數值不硬寫在 Blueprint。

#### FW-MVP-0301：建立 `AFWVehicleBase`

優先級：P0

產出：

- HealthComponent
- StateMachineComponent
- EnterVehicle
- ExitVehicle
- HandleVehicleDestroyed

驗收條件：

- 載具可被玩家互動。
- 載具生命歸零進入 `Destroyed`。
- 毀損事件只觸發一次。

#### FW-MVP-0302：建立車子

優先級：P0

產出：

- `BP_Vehicle_Car`
- `DA_Vehicle_Car_Balanced`

驗收條件：

- 車子可駕駛。
- 車子可受傷與毀損。
- 玩家可進出。

#### FW-MVP-0303：建立機車

優先級：P0

產出：

- `BP_Vehicle_Motorcycle`
- `DA_Vehicle_Motorcycle_Fast`

驗收條件：

- 機車可駕駛。
- 機車速度高於車子。
- 機車生命低於車子。

#### FW-MVP-0304：實作核心載具歸屬

優先級：P0

產出：

- PlayerState.CoreVehicle
- Vehicle.OwnerController
- CoreVehicleDestroyed 事件

驗收條件：

- 玩家有一台核心載具。
- 核心載具毀損可被 RuleSystem 偵測。

### 9.3 Phase 3 Gate

必須達成：

- 玩家可上下載具。
- 車子與機車可駕駛。
- 載具可被武器摧毀。
- 核心載具事件可觸發。

## 10. Phase 4：規則、命數、重生

### 10.1 目標

完成 MVP 的比賽流程：開始、扣命、重生、結束。

### 10.2 任務

#### FW-MVP-0400：建立 `UFWMatchRuleSet`

優先級：P0

產出：

- AIEnemyCount
- PlayerLives
- bRespawnEnabled
- RespawnDelay
- bCoreVehicleEnabled
- CoreVehiclePenalty
- bVehicleStealingEnabled

驗收條件：

- 預設規則為核心載具摧毀扣一條命。
- 可建立不同 RuleSet Data Asset。

#### FW-MVP-0401：建立 `AFWCompetitiveGameMode`

優先級：P0

產出：

- StartMatchFlow
- EndMatchFlow
- RequestRespawn
- HandleCoreVehicleDestroyed

驗收條件：

- 比賽可開始。
- 核心載具摧毀時玩家扣命。
- 命數歸零時比賽結束。

#### FW-MVP-0402：建立 Match StateMachine

優先級：P0

產出：

- WaitingForStart
- Spawning
- InProgress
- Ending
- Ended

驗收條件：

- 只有 `InProgress` 允許正式扣命。
- `Ended` 後不再套用新傷害結果。

#### FW-MVP-0403：實作重生流程

優先級：P0

產出：

- Respawn Timer
- Respawn Point
- RespawnCompleted 事件

驗收條件：

- 玩家死亡或扣命後可重生。
- 重生後狀態回到 `OnFoot`。
- 重生不重置整場比賽。

### 10.3 Phase 4 Gate

必須達成：

- 比賽流程可完整跑完。
- 核心載具扣命規則可用。
- 重生可用。
- 比賽可結束。

## 11. Phase 5：AI 原型

### 11.1 目標

完成多個 AI 敵人的基本戰鬥能力，至少能巡邏、偵測、追擊、攻擊玩家。

### 11.2 任務

#### FW-MVP-0500：建立 `AFWAICharacter`

優先級：P0

產出：

- AI 角色基底
- HealthComponent
- Weapon 裝備能力
- StateMachineComponent

驗收條件：

- AI 可被生成。
- AI 可被武器傷害。
- AI 死亡觸發事件。

#### FW-MVP-0501：建立 `AFWEnemyAIController`

優先級：P0

產出：

- AI Controller
- Perception 或簡化偵測
- Blackboard
- Behavior Tree

驗收條件：

- AI 可偵測玩家。
- AI 可追擊玩家。
- AI 可攻擊玩家。

#### FW-MVP-0502：建立 AI 狀態

優先級：P0

產出：

- Idle
- Patrol
- Alert
- AttackOnFoot
- SearchVehicle
- DriveVehicle
- Dead

驗收條件：

- Debug 可顯示 AI 目前狀態。
- AI 狀態變更會觸發事件。

#### FW-MVP-0503：AI 使用武器

優先級：P0

產出：

- AI 裝備步槍
- AI 開火節奏
- 命中率參數

驗收條件：

- AI 可對玩家造成傷害。
- 難度可影響 AI 命中率或反應時間。

#### FW-MVP-0504：AI 載具意圖原型

優先級：P1

產出：

- AI 尋找附近載具
- AI 進入載具或追逐載具目標

驗收條件：

- 至少一個 AI 行為能展示載具相關決策。
- 若載具流程不穩，不阻塞 MVP 戰鬥循環。

### 11.3 Phase 5 Gate

必須達成：

- 多個 AI 可生成。
- AI 可偵測、追擊、攻擊。
- AI 可被擊敗。
- AI 狀態可 Debug。

## 12. Phase 6：測試地圖與玩法整合

### 12.1 目標

用小型測試地圖打通完整核心循環，不進入完整開放世界製作。

### 12.2 任務

#### FW-MVP-0600：建立 `L_Test_CombatGarage`

優先級：P0

產出：

- 玩家出生點
- 3 個 AI 出生點
- 車子出生點
- 機車出生點
- 掩體
- 直線道路
- 坡道
- 武器測試區

驗收條件：

- 可以在 3 分鐘內測完玩家、武器、載具、AI、規則核心流程。

#### FW-MVP-0601：整合完整遊戲循環

優先級：P0

產出：

- Spawn
- Move
- Shoot
- Enter Vehicle
- Damage Vehicle
- Destroy Core Vehicle
- Lose Life
- Respawn
- End Match

驗收條件：

- 不需手動重設關卡即可完成一場測試比賽。
- Debug 可看出每個關鍵狀態。

### 12.3 Phase 6 Gate

必須達成：

- 完整核心循環可在測試地圖中跑完。
- 不允許此階段開始完整大地圖製作。

## 13. Phase 7：UI、設定、Loadout

### 13.1 目標

建立最小 UI 流程，支援比賽設定與比賽前武器配置。

### 13.2 任務

#### FW-MVP-0700：建立 HUD

優先級：P0

產出：

- 玩家生命
- 命數
- 目前武器
- 彈藥
- 載具生命
- 核心載具生命

驗收條件：

- HUD 只顯示狀態，不直接修改遊戲結果。

#### FW-MVP-0701：建立 Match Settings

優先級：P1

產出：

- AI 數量
- 難度
- 玩家命數
- 重生開關
- 核心載具規則

驗收條件：

- 設定會生成或套用到 `UFWMatchRuleSet`。

#### FW-MVP-0702：建立 Weapon Loadout

優先級：P1

產出：

- 選擇手槍、步槍、狙擊槍
- 顯示數值
- 儲存配置

驗收條件：

- 比賽開始前可選擇武器。
- Loadout 儲存 ContentId。

### 13.3 Phase 7 Gate

必須達成：

- HUD 可用。
- 規則設定與 Loadout 至少有最小可用流程。

## 14. Phase 8：Debug、測試、效能與穩定化

### 14.1 目標

讓 MVP 可反覆測試、可定位問題、可穩定展示。

### 14.2 任務

#### FW-MVP-0800：建立手動測試清單

優先級：P0

產出：

- 玩家測試
- 武器測試
- 載具測試
- AI 測試
- 規則測試
- 重生測試

驗收條件：

- 每次 MVP 測試都能照表執行。

#### FW-MVP-0801：建立 Smoke Test

優先級：P0

產出：

- 10 分鐘內可完成的核心測試流程

驗收條件：

- 每次重大修改後都可快速確認核心玩法沒壞。

#### FW-MVP-0802：完善 Debug HUD

優先級：P1

產出：

- 玩家狀態
- 武器狀態
- 載具狀態
- AI 狀態
- 規則狀態
- 事件 Log

驗收條件：

- 遇到核心循環問題時，不需要猜目前狀態。

#### FW-MVP-0803：效能初檢

優先級：P1

產出：

- AI 數量測試
- 載具數量測試
- 武器特效數量測試
- Tick 使用檢查

驗收條件：

- 測試地圖在目標機器上可穩定運行。
- 沒有明顯每 frame 昂貴 UI 查詢。

### 14.3 Phase 8 Gate

必須達成：

- 核心功能可重複測試。
- Debug 足以定位問題。
- MVP 可穩定展示。

## 15. Phase 9：MVP 驗收

### 15.1 最終驗收清單

MVP 驗收必須全部通過：

- 玩家可出生。
- 玩家可走、跑、跳、蹲、爬行。
- 玩家可切換視角。
- 玩家可用三把武器。
- 武器可傷害 AI。
- 武器可傷害載具。
- 玩家可上下載具。
- 車子可駕駛。
- 機車可駕駛。
- 載具可毀損。
- 核心載具毀損會扣命。
- 玩家可重生。
- 命數歸零會結束比賽。
- 多個 AI 可生成。
- AI 可追擊並攻擊玩家。
- HUD 顯示必要資訊。
- Debug HUD 顯示核心狀態。
- 所有核心內容有 ContentId。
- 測試地圖可完整展示玩法循環。

### 15.2 不屬於 MVP 的項目

以下項目不要阻塞 MVP：

- 完整大地圖
- 完整美術
- 完整音效
- 線上多人
- 分割螢幕
- 天氣系統
- 船
- 角色造型系統
- 完整武器升級樹
- 完整反作弊
- 完整自動化測試

## 16. 嚴格開發規則

### 16.1 不提前做完整大地圖

核心循環未通過 Phase 6 Gate 前，不製作完整開放世界。

### 16.2 不提前增加武器數量

三把 MVP 武器穩定前，不新增第四把武器。

### 16.3 不讓 UI 控制遊戲結果

UI 只能發出請求或顯示狀態，不直接扣血、扣命、摧毀載具或結束比賽。

### 16.4 不繞過 ContentId

武器、載具、規則、難度、角色都必須有穩定 ContentId。

### 16.5 不讓單機原型破壞多人未來

即使 MVP 是單機，也要遵守 Server-authoritative 命名與責任邊界。

### 16.6 不接受沒有驗收條件的任務

每個任務都必須能回答：

- 做完會產生什麼？
- 怎樣算完成？
- 怎樣測？

## 17. 建議第一週開發順序

第一週只做地基，不碰大地圖與美術。

建議順序：

1. 建立 UE5 C++ 專案。
2. 建立 Source / Content 結構。
3. 加入 Enhanced Input 與 Build.cs 依賴。
4. 建立 `UFWPrimaryDataAsset`。
5. 建立 `UFWHealthComponent`。
6. 建立 `UFWStateMachineComponent`。
7. 建立 `FFWGameplayEvent` 與 `UFWEventSubsystem` 最小版。
8. 建立 `AFWPlayerCharacter`。
9. 建立 `L_Test_CombatGarage` 空白測試地圖。
10. 完成玩家走、跑、跳、蹲、爬行。

## 18. 下一步

任務清單建立後，下一步才是建立 UE5 專案。

如果尚未安裝 Unreal Engine 5，先完成：

- 安裝 Epic Games Launcher
- 安裝 UE5
- 安裝 Visual Studio 與 C++ 工具鏈
- 建立 `FW` C++ Third Person 專案

如果 UE5 已安裝，下一步直接執行 Phase 0。

