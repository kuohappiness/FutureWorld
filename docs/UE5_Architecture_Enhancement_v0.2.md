# Unreal Engine 5 架構補強藍圖 v0.2

## 1. 文件目的

本文件是 `UE5_Game_Design_and_Technical_Architecture_v0.1.md` 與 `UE5_Project_Structure_and_Cpp_Class_Skeleton_v0.1.md` 的補強版。

v0.1 已經足夠進入原型開發，但若目標是長期擴充成開放世界、載具戰鬥、多人連線、分割螢幕、武器改裝、天氣系統與多遊戲模式，還需要補上幾個橫向底層系統。

v0.2 的核心目標：

- 減少系統之間硬耦合
- 讓內容資料可長期擴充
- 從第一天就預留多人連線權威規則
- 讓 Debug 與測試能力成為架構的一部分
- 讓玩家、AI、載具、比賽流程都有清楚狀態機
- 避免未來從原型轉正式製作時大幅重寫

## 2. v0.2 新增系統總覽

```text
FW v0.2 Enhancement
├─ EventSystem
│  ├─ FWEventSubsystem
│  ├─ FWGameplayEvent
│  ├─ FWCombatEvent
│  ├─ FWVehicleEvent
│  └─ FWMatchEvent
│
├─ AssetManagementSystem
│  ├─ FWAssetManager
│  ├─ FWContentId
│  ├─ FWDataRegistry
│  └─ FWPrimaryDataAsset
│
├─ SaveProfileSystem
│  ├─ FWPlayerProfile
│  ├─ FWGameSettingsSave
│  ├─ FWLoadoutSave
│  └─ FWProgressionSave
│
├─ NetworkAuthoritySystem
│  ├─ ServerAuthoritativeDamage
│  ├─ ServerAuthoritativeVehicleHealth
│  ├─ ServerValidatedWeaponFire
│  ├─ ReplicatedMatchState
│  └─ ClientPredictionPolicy
│
├─ DebugToolSystem
│  ├─ FWDebugSubsystem
│  ├─ FWDebugCheatManager
│  ├─ FWVehicleDebug
│  ├─ FWWeaponDebug
│  ├─ FWAIStateDebug
│  └─ FWRuleDebug
│
├─ StateMachineSystem
│  ├─ FWStateMachineComponent
│  ├─ FWPlayerStateMachine
│  ├─ FWVehicleStateMachine
│  ├─ FWAIStateMachine
│  └─ FWMatchStateMachine
│
├─ GameplayTagSystem
│  ├─ Character Tags
│  ├─ Weapon Tags
│  ├─ Vehicle Tags
│  ├─ Damage Tags
│  └─ Rule Tags
│
├─ ProgressionSystem
│  ├─ FWUnlockDefinition
│  ├─ FWUpgradeDefinition
│  ├─ FWProgressionComponent
│  └─ FWRewardTable
│
├─ AudioSystem
│  ├─ FWAudioSubsystem
│  ├─ FWAudioCueProfile
│  ├─ Vehicle Audio States
│  └─ Combat Audio States
│
└─ PerformanceBudgetSystem
   ├─ FWPerformanceBudgetProfile
   ├─ AI Budget
   ├─ Vehicle Budget
   ├─ Weapon FX Budget
   └─ Open World Streaming Budget
```

## 3. EventSystem

### 3.1 為什麼需要

目前 v0.1 的模組已經切開，但還缺少標準化溝通方式。

如果沒有事件系統，未來很容易出現：

- Weapon 直接呼叫 UI
- Vehicle 直接呼叫 GameMode、AI、HUD
- AI 直接修改玩家狀態
- RuleSystem 到處監聽硬連結
- 載具爆炸後需要手動通知十幾個系統

EventSystem 的目標是讓系統之間用事件溝通，而不是互相硬接。

### 3.2 建議類別

```text
Source/FW/Events
├─ FWEventSubsystem.h
├─ FWGameplayEventTypes.h
├─ FWCombatEventTypes.h
├─ FWVehicleEventTypes.h
├─ FWMatchEventTypes.h
└─ FWEventListenerComponent.h
```

### 3.3 核心事件類型

Combat 事件：

- `WeaponFired`
- `ProjectileSpawned`
- `HitConfirmed`
- `DamageApplied`
- `HealthChanged`
- `ActorKilled`

Vehicle 事件：

- `VehicleEntered`
- `VehicleExited`
- `VehicleDamaged`
- `VehicleDisabled`
- `VehicleDestroyed`
- `CoreVehicleDestroyed`

Match 事件：

- `MatchStarted`
- `MatchPaused`
- `LifeLost`
- `RespawnStarted`
- `RespawnCompleted`
- `MatchEnded`

AI 事件：

- `AIDetectedTarget`
- `AILostTarget`
- `AIEnteredVehicle`
- `AIChangedState`

### 3.4 建議資料結構

```cpp
USTRUCT(BlueprintType)
struct FFWGameplayEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FGameplayTag EventTag;

    UPROPERTY(BlueprintReadWrite)
    TObjectPtr<AActor> Instigator = nullptr;

    UPROPERTY(BlueprintReadWrite)
    TObjectPtr<AActor> Target = nullptr;

    UPROPERTY(BlueprintReadWrite)
    float Magnitude = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    FVector WorldLocation = FVector::ZeroVector;
};
```

### 3.5 使用原則

- 戰鬥、載具、AI、UI、規則系統都可以監聽事件。
- 事件不能取代權威邏輯，尤其是多人連線下的傷害判定。
- GameMode 仍負責比賽規則決策。
- UI 只監聽事件更新顯示，不應控制遊戲狀態。

## 4. AssetManagementSystem

### 4.1 為什麼需要

本遊戲會有大量可擴充內容：

- 角色
- 角色外觀
- 武器
- 武器模組
- 武器升級
- 載具
- AI 類型
- 難度設定
- 比賽規則
- 天氣設定

如果到處直接引用 Data Asset，後期會很難處理：

- 載入時機
- DLC 或模組化內容
- 多人同步 ID
- 內容版本管理
- 缺失資產 fallback

### 4.2 建議類別

```text
Source/FW/Assets
├─ FWAssetManager.h
├─ FWContentId.h
├─ FWPrimaryDataAsset.h
├─ FWDataRegistrySubsystem.h
└─ FWAssetLoadHandle.h
```

### 4.3 ContentId 原則

所有核心內容都應有穩定 ID。

範例：

```text
Character.Player.Default
Character.AI.TacticalRaider
Weapon.Pistol.Basic
Weapon.Rifle.Basic
Weapon.Sniper.Basic
Vehicle.Car.Balanced
Vehicle.Motorcycle.Fast
RuleSet.Competitive.Default
Difficulty.Normal
Weather.Rain.Light
```

### 4.4 建議基底 Data Asset

```cpp
UCLASS(Abstract, BlueprintType)
class FW_API UFWPrimaryDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Content")
    FName ContentId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Content")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Content")
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Content")
    FGameplayTagContainer ContentTags;
};
```

### 4.5 使用原則

- 多人同步時同步 `ContentId`，不要同步硬資產引用。
- UI 顯示資料時透過 Registry 查詢。
- GameMode 套用規則時透過 `UFWMatchRuleSet` 的 ID 查找。
- 武器配置儲存時保存 ID，不保存暫時物件指標。

## 5. SaveProfileSystem

### 5.1 為什麼需要

目前 v0.1 只有 `SaveGame` 概念，但本遊戲需要分層保存不同資料。

如果全部塞進同一個 SaveGame，後期會難以處理：

- 玩家設定
- 武器配置
- 已解鎖內容
- 遊戲進度
- 控制器設定
- 未來多人帳號資料

### 5.2 建議拆分

```text
SaveProfileSystem
├─ FWPlayerProfile
├─ FWGameSettingsSave
├─ FWLoadoutSave
├─ FWProgressionSave
└─ FWSaveSubsystem
```

### 5.3 各存檔責任

`FWPlayerProfile`：

- 玩家名稱
- 偏好角色
- 偏好載具
- 偏好武器配置

`FWGameSettingsSave`：

- 畫面設定
- 音效設定
- 輸入設定
- 語言設定
- 無障礙設定

`FWLoadoutSave`：

- 主要武器 ID
- 副武器 ID
- 裝備模組 ID
- 升級等級

`FWProgressionSave`：

- 已解鎖角色
- 已解鎖武器
- 已解鎖載具
- 已解鎖模組
- 經驗值或貨幣

### 5.4 存檔原則

- 儲存 `ContentId`，不要儲存直接資產引用。
- 設定與進度分開，避免重置設定時誤刪進度。
- MVP 可以先本機保存，未來再接線上帳號。

## 6. NetworkAuthoritySystem

### 6.1 為什麼需要

雖然 MVP 是單機對 AI，但本遊戲未來明確要支援線上多人。

如果原型階段完全用單機思維寫，未來擴充線上多人時會遇到：

- 傷害由客戶端決定，容易作弊
- UI 控制遊戲規則
- 載具生命值不同步
- 武器命中結果不同步
- AI 跑在每個客戶端導致狀態不一致

### 6.2 權威規則

從第一天開始遵守：

- GameMode 只存在於 Server，負責比賽規則。
- GameState 負責同步比賽狀態。
- PlayerState 負責同步分數、命數、隊伍、核心載具。
- 傷害結果由 Server 決定。
- 載具生命由 Server 決定。
- 核心載具毀損懲罰由 Server 決定。
- AI 只在 Server 執行決策。
- Client 可以播放預測特效，但不能決定正式結果。
- UI 不得直接修改正式遊戲狀態。

### 6.3 武器開火流程

建議流程：

1. Client 按下開火。
2. Client 立即播放本地預測動畫、音效、槍口火光。
3. Client 發送 Server RPC：請求開火。
4. Server 驗證：
   - 是否持有該武器
   - 是否有彈藥
   - 射速是否允許
   - 是否處於可開火狀態
5. Server 計算命中或生成投射物。
6. Server 套用傷害。
7. Server 通知相關 Client 播放命中特效。

### 6.4 載具傷害流程

建議流程：

1. Weapon 或 Collision 產生傷害請求。
2. Server 驗證傷害來源。
3. Server 查詢 HitZone。
4. Server 計算最終傷害。
5. Server 修改 Vehicle Health。
6. Vehicle Health 透過 Replication 更新。
7. 若核心載具被摧毀，GameMode 觸發扣命或失敗規則。

### 6.5 MVP 實作策略

即使先做單機，也建議：

- 命名上區分 `Request`、`Server`、`Multicast`、`Local`。
- 傷害集中走 `ApplyDamageRequest`。
- 玩家命數只由 GameMode 或 RuleSystem 修改。
- UI 透過事件或狀態讀取，不直接扣血或扣命。

## 7. DebugToolSystem

### 7.1 為什麼需要

本遊戲有多種高互動系統：玩家、AI、載具、武器、規則、重生、核心載具。沒有 Debug 工具，後期會很難排查問題。

Debug 系統不是附屬品，而是開發效率的核心。

### 7.2 建議類別

```text
Source/FW/Debug
├─ FWDebugSubsystem.h
├─ FWDebugCheatManager.h
├─ FWDebugDrawComponent.h
├─ FWVehicleDebugLibrary.h
├─ FWWeaponDebugLibrary.h
├─ FWAIStateDebugLibrary.h
└─ FWRuleDebugLibrary.h
```

### 7.3 必備 Debug 功能

玩家：

- 顯示玩家目前狀態
- 顯示目前攝影機模式
- 顯示目前互動目標
- 顯示輸入 Context

武器：

- 顯示目前武器資料
- 顯示彈藥
- 顯示射線或投射物路徑
- 顯示命中區域
- 顯示傷害結果

載具：

- 顯示載具生命值
- 顯示弱點部位
- 顯示座位資訊
- 顯示駕駛者
- 顯示核心載具擁有者
- 強制摧毀測試

AI：

- 顯示目前 Behavior State
- 顯示目標
- 顯示感知範圍
- 顯示是否想找載具
- 顯示難度參數

規則：

- 顯示目前比賽狀態
- 顯示玩家命數
- 顯示重生計時
- 顯示勝敗條件
- 顯示核心載具規則

### 7.4 Debug 命令範例

```text
fw.debug.ai 1
fw.debug.vehicle 1
fw.debug.weapon 1
fw.debug.rules 1
fw.spawn.ai TacticalRaider 3
fw.spawn.vehicle Vehicle.Car.Balanced
fw.damage.corevehicle 500
fw.match.forceend
```

## 8. StateMachineSystem

### 8.1 為什麼需要

玩家、AI、載具、比賽流程都會有複雜狀態。

如果只用大量 bool 控制，例如：

- `bIsDead`
- `bIsDriving`
- `bIsEnteringVehicle`
- `bIsAiming`
- `bIsRespawning`

很快會出現互相衝突的狀態。

### 8.2 建議類別

```text
Source/FW/State
├─ FWStateMachineComponent.h
├─ FWStateDefinition.h
├─ FWStateTransition.h
├─ FWPlayerStateTypes.h
├─ FWVehicleStateTypes.h
├─ FWAIStateTypes.h
└─ FWMatchStateTypes.h
```

### 8.3 玩家狀態

```text
OnFoot
Aiming
Crouching
Crawling
Interacting
EnteringVehicle
InVehicle
ExitingVehicle
Dead
Respawning
Spectating
```

重要規則：

- `Dead` 不可開火。
- `EnteringVehicle` 不可切換武器。
- `InVehicle` 使用 Vehicle Input Context。
- `Respawning` 由 RuleSystem 控制。

### 8.4 載具狀態

```text
Idle
Occupied
Damaged
Disabled
Destroyed
Respawning
Wrecked
```

重要規則：

- `Disabled` 可以保留乘客，但不可正常駕駛。
- `Destroyed` 會觸發 VehicleDestroyed 事件。
- 若是核心載具，`Destroyed` 會觸發 CoreVehicleDestroyed。

### 8.5 AI 狀態

```text
Idle
Patrol
Alert
Investigate
AttackOnFoot
SearchVehicle
EnterVehicle
DriveVehicle
VehicleAttack
Retreat
Dead
Respawning
```

重要規則：

- AI 決策在 Server 執行。
- AI 狀態可透過 Debug HUD 顯示。
- Behavior Tree 可使用狀態機結果作為 Blackboard 資料。

### 8.6 比賽狀態

```text
WaitingForStart
LoadoutSelection
Spawning
InProgress
Paused
Ending
Ended
```

重要規則：

- 只有 `InProgress` 允許正式計分與扣命。
- `Ending` 不再接受新傷害結果。
- `Ended` 顯示結算 UI。

## 9. GameplayTagSystem

### 9.1 為什麼需要

Gameplay Tags 可以讓內容、規則與狀態更容易組合。

適合用於：

- 武器分類
- 傷害類型
- 載具類型
- 狀態效果
- AI 行為分類
- 地圖區域分類
- 比賽規則條件

### 9.2 建議 Tag 命名

```text
Character.Player
Character.AI
Character.AI.TacticalRaider

Weapon.Pistol
Weapon.Rifle
Weapon.Sniper
Weapon.VehicleMounted

Vehicle.Car
Vehicle.Motorcycle
Vehicle.Boat
Vehicle.Core

Damage.Bullet
Damage.Explosion
Damage.Collision
Damage.Fall
Damage.Weather

State.Player.OnFoot
State.Player.InVehicle
State.Player.Dead

State.Vehicle.Disabled
State.Vehicle.Destroyed

Rule.Respawn.Enabled
Rule.CoreVehicle.Enabled
Rule.VehicleStealing.Enabled

Weather.Rain
Weather.Fog
Weather.Storm
```

### 9.3 使用原則

- Tag 用於分類與條件判斷。
- 不要用大量 enum 取代所有 Tag。
- 穩定、固定、少量狀態可用 enum。
- 可擴充、內容驅動的分類用 Gameplay Tag。

## 10. ProgressionSystem

### 10.1 為什麼需要

雖然 MVP 可先開放所有內容，但未來武器模組、升級、角色、載具很可能需要解鎖或進度系統。

建議提前保留架構，避免後期重構 Loadout 與 Save。

### 10.2 建議類別

```text
Source/FW/Progression
├─ FWUnlockDefinition.h
├─ FWUpgradeDefinition.h
├─ FWProgressionComponent.h
├─ FWRewardTable.h
└─ FWProgressionSubsystem.h
```

### 10.3 可解鎖內容

- 武器
- 武器模組
- 武器升級
- 載具
- 載具塗裝
- 角色
- 角色外觀
- 遊戲模式
- 難度

### 10.4 MVP 原則

MVP 可以先全部開放，但資料結構要支援：

- 是否已解鎖
- 解鎖條件
- 升級等級
- 成本
- 前置需求

## 11. AudioSystem

### 11.1 為什麼需要

本遊戲有大量狀態變化，音效應該跟著系統資料走，而不是散落在每個 Blueprint 裡。

### 11.2 建議類別

```text
Source/FW/Audio
├─ FWAudioSubsystem.h
├─ FWAudioCueProfile.h
├─ FWVehicleAudioComponent.h
├─ FWWeaponAudioComponent.h
└─ FWCombatAudioLibrary.h
```

### 11.3 音效分類

武器：

- 開火
- 換彈
- 空彈
- 命中角色
- 命中載具
- 命中弱點

載具：

- 引擎啟動
- 加速
- 煞車
- 撞擊
- 受損
- 癱瘓
- 爆炸

角色：

- 腳步
- 跳躍
- 落地
- 受傷
- 死亡
- 重生

UI：

- 選單確認
- 錯誤
- 武器配置
- 比賽開始
- 比賽結束

## 12. PerformanceBudgetSystem

### 12.1 為什麼需要

開放世界 + 多 AI + 載具 + 武器特效很容易超出效能預算。

架構上應提前定義效能預算，而不是最後才優化。

### 12.2 預算分類

建議追蹤：

- 同時活躍 AI 數量
- 同時活躍載具數量
- 同時投射物數量
- 武器特效數量
- 爆炸特效數量
- 世界分區載入數量
- UI 更新頻率
- Tick 使用量

### 12.3 建議規則

- 不重要的遠距 AI 降低 Tick 頻率。
- 遠距載具使用簡化更新。
- 武器特效使用池化。
- 投射物與爆炸特效設定上限。
- UI 不應每 frame 做昂貴查詢。
- 大地圖先做 Blockout，再逐步加入細節。

## 13. v0.2 後的高階架構

```text
FW
├─ Core
├─ CharacterSystem
├─ ActionSystem
├─ VehicleSystem
├─ WeaponSystem
├─ CombatSystem
├─ AISystem
├─ WorldSystem
├─ InputSystem
├─ CameraSystem
├─ RuleSystem
├─ DifficultySystem
├─ UI
│
├─ EventSystem
├─ AssetManagementSystem
├─ SaveProfileSystem
├─ NetworkAuthoritySystem
├─ DebugToolSystem
├─ StateMachineSystem
├─ GameplayTagSystem
├─ ProgressionSystem
├─ AudioSystem
└─ PerformanceBudgetSystem
```

## 14. Project Structure 補強

建議在 `Source/FW` 中新增：

```text
Source/FW/
├─ Events/
├─ Assets/
├─ Save/
├─ Network/
├─ Debug/
├─ State/
├─ Tags/
├─ Progression/
├─ Audio/
└─ Performance/
```

建議在 `Content/FW` 中新增：

```text
Content/FW/
├─ Data/
│  ├─ Registry/
│  ├─ Tags/
│  ├─ Progression/
│  └─ AudioProfiles/
│
├─ Debug/
│  ├─ Widgets/
│  └─ Materials/
│
└─ Developer/
   ├─ TestMaps/
   ├─ DebugBlueprints/
   └─ PrototypeAssets/
```

## 15. MVP 實作優先順序調整

v0.1 的 MVP 順序仍然有效，但建議加入以下底層任務。

### 15.1 Phase 0.5：基礎橫向系統

在正式做武器、載具、AI 前，先建立：

1. `FWGameplayEvent` 基本資料結構
2. `FWEventSubsystem` 最小可用版本
3. `FWPrimaryDataAsset` 基底
4. `ContentId` 命名規則
5. `FWHealthComponent` 的 Server-authoritative 設計原則
6. `FWStateMachineComponent` 最小版本
7. Debug 開關與 Debug HUD 佔位

### 15.2 Phase 1：玩家原型補強

除了原本玩家移動，加入：

- Player StateMachine
- OnFoot / InVehicle / Dead / Respawning 狀態
- Debug 顯示目前玩家狀態
- Input Context 顯示

### 15.3 Phase 2：武器原型補強

除了原本武器原型，加入：

- Weapon ContentId
- WeaponFired 事件
- HitConfirmed 事件
- Server 驗證流程命名
- Weapon Debug 顯示

### 15.4 Phase 3：載具原型補強

除了原本載具原型，加入：

- Vehicle ContentId
- Vehicle StateMachine
- VehicleDamaged 事件
- VehicleDestroyed 事件
- CoreVehicleDestroyed 事件
- Vehicle Debug 顯示

### 15.5 Phase 4：AI 原型補強

除了原本 AI 原型，加入：

- AI StateMachine
- AIChangedState 事件
- AI Debug 顯示
- AI 難度 Profile ID

### 15.6 Phase 5：規則系統補強

除了原本規則系統，加入：

- Match StateMachine
- MatchStarted / LifeLost / RespawnCompleted / MatchEnded 事件
- Rule Debug 顯示
- RuleSet ContentId

## 16. 最小可用 v0.2 實作範圍

不要一次做完整大系統。v0.2 的第一步只需要最小版本。

### 16.1 EventSystem 最小版

需要：

- 可以廣播事件
- 可以 Blueprint 監聽事件
- 支援 GameplayTag 作為事件 ID
- 支援 Instigator、Target、Magnitude、WorldLocation

暫時不需要：

- 複雜佇列
- 優先級
- 跨關卡事件持久化
- 網路事件重播

### 16.2 AssetManagement 最小版

需要：

- 所有主要 Data Asset 有 `ContentId`
- 武器、載具、規則、難度使用 ID 命名
- 存檔保存 ID

暫時不需要：

- 完整 DLC 系統
- 熱更新
- 外部模組載入

### 16.3 NetworkAuthority 最小版

需要：

- 傷害走單一入口
- 扣命走 GameMode / RuleSystem
- 載具生命集中管理
- UI 不直接修改遊戲狀態

暫時不需要：

- 真正上線多人
- Matchmaking
- Lag compensation
- 反作弊

### 16.4 Debug 最小版

需要：

- 開關 Debug 顯示
- 顯示玩家狀態
- 顯示載具生命
- 顯示武器資訊
- 顯示 AI 狀態
- 顯示規則狀態

暫時不需要：

- 完整 Debug UI 編輯器
- 圖表分析
- 自動測試報告

## 17. 必須避免的架構陷阱

### 17.1 UI 控制遊戲結果

錯誤：

- Widget 按鈕直接扣玩家命數
- HUD 直接摧毀載具

正確：

- Widget 發出請求
- GameMode / RuleSystem 判斷
- GameState / PlayerState 同步結果
- UI 只顯示結果

### 17.2 武器直接控制規則

錯誤：

- Weapon 命中核心載具後直接判定玩家輸

正確：

- Weapon 造成傷害
- Vehicle Health 歸零
- Vehicle 發出 `CoreVehicleDestroyed`
- GameMode 根據 RuleSet 扣命或判敗

### 17.3 AI 與玩家使用不同規則

除非設計明確指定，AI 應盡量走與玩家相同的生命、載具、武器、規則流程。

這樣未來改多人時，AI 可以更容易替換成人類玩家。

### 17.4 內容沒有穩定 ID

錯誤：

- 存檔直接記錄某個 Blueprint 路徑

正確：

- 存檔記錄 `Weapon.Rifle.Basic`
- AssetRegistry 查詢對應 Data Asset

### 17.5 過早做完整開放世界

先完成 `L_Test_CombatGarage` 的完整循環，再進入大地圖。

核心循環未完成前，大地圖只會放大問題。

## 18. v0.2 完成標準

當以下條件達成時，架構可視為 v0.2 完成：

- 主要系統可以透過 EventSystem 溝通。
- 武器、載具、規則、難度有穩定 ContentId。
- 傷害、載具生命、扣命流程有 Server-authoritative 設計。
- 玩家、載具、AI、比賽流程有明確狀態機。
- Debug HUD 可以顯示玩家、武器、載具、AI、規則狀態。
- Save/Profile 架構清楚區分設定、配置與進度。
- UI 不直接控制正式遊戲結果。
- `L_Test_CombatGarage` 可以驗證完整戰鬥循環。

## 19. 建議下一步

下一份文件建議建立：

- `UE5_MVP_Implementation_Tasks_v0.2.md`

內容應把 v0.1 與 v0.2 整合成可執行任務：

- Phase 0：UE5 專案建立
- Phase 0.5：橫向底層系統
- Phase 1：玩家與狀態機
- Phase 2：武器與事件
- Phase 3：載具與核心載具規則
- Phase 4：AI 與 Debug
- Phase 5：規則、重生、勝敗
- Phase 6：測試地圖與第一個可玩版本

