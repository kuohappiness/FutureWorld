# Unreal Engine 5 遊戲設計與技術架構 v0.1

## 1. 專案願景

### 1.1 遊戲摘要

本專案是一款使用 Unreal Engine 5 製作的卡通風格 3D 開放世界載具戰鬥遊戲。

玩家可以在大型混合開放世界地圖中自由移動，進行步行戰鬥，使用 `F` 鍵進入或離開載具，在比賽前自訂武器，並與多個 AI 敵人交戰。預設遊戲模式是偏戰術感的對戰競技，但整體架構必須保留未來擴充成線上多人、單機分割螢幕、其他遊戲模式與天氣玩法的彈性。

### 1.2 核心定位

這款遊戲應該帶給玩家以下感受：

- GTA 式開放世界混戰
- 戰術感的載具與步兵戰鬥
- 卡通風格視覺
- 可長期擴充的模組化系統
- 可在比賽前自訂的競技規則

### 1.3 設計支柱

1. 開放世界戰術戰鬥
   - 地圖需要支援多路線、掩體、伏擊點、載具路線與開放戰鬥區域。

2. 步兵與載具雙核心戰鬥
   - 玩家與 AI 可以在步行狀態或載具內戰鬥。
   - 人可以攻擊載具。
   - 載具也可以攻擊人。

3. 模組化擴充
   - 角色、動作、載具、武器、AI、難度、遊戲規則、天氣與遊戲模式都應該模組化。

4. 比賽設定彈性
   - 重生規則、載具毀損規則、命數、難度、武器配置、AI 數量與未來模式選項都應該可調整。

5. 預留多人連線架構
   - 第一版是玩家對 AI，但架構上應避免做出未來阻礙線上多人擴充的決定。

## 2. 目標引擎

### 2.1 引擎選擇

引擎：Unreal Engine 5

建議開發模式：

- C++ 負責核心系統
- Blueprint 負責內容配置、數值調整與設計師可操作邏輯
- Data Assets 與 Data Tables 負責遊戲資料
- Enhanced Input 負責輸入映射
- World Partition 負責大型開放世界串流載入
- Gameplay Ability System 或能力式模組元件負責可擴充的玩家行動與戰鬥效果

### 2.2 為什麼選 Unreal Engine 5

建議使用 Unreal Engine 5，因為本遊戲需要：

- 大型開放世界支援
- 第三人稱動作玩法
- 載具玩法
- 預留多人連線的架構
- 成熟的 AI 工具
- 高品質卡通渲染能力
- 資料驅動的模組化玩法
- 分割螢幕與控制器擴充潛力

## 3. 遊戲範圍

### 3.1 MVP 範圍

第一個可玩版本應包含：

- 一張大型混合開放世界地圖
- 預設第三人稱視角
- 可切換第一人稱視角
- 玩家移動：
  - 走路
  - 跑步
  - 跳躍
  - 蹲下
  - 爬行
- 使用 `F` 鍵進入與離開載具
- 兩種載具：
  - 車子
  - 機車
- 三種武器：
  - 手槍
  - 步槍
  - 狙擊槍
- 比賽前武器自訂
- 多個 AI 敵人
- 一種 AI 敵人原型
- 載具生命值與毀損
- 玩家命數與重生
- 預設規則：核心載具被摧毀時扣一條命
- 比賽設定選單
- 偏戰術感的戰鬥節奏
- 卡通風格角色與環境

### 3.2 MVP 後擴充

未來擴充方向：

- 船類載具
- 更多載具種類
- 線上多人遊玩
- 單機分割螢幕
- 天氣系統
- 游泳
- 攀爬
- 類似蜘蛛人的擺盪移動
- NPC 互動
- 更多 AI 類型
- 多種遊戲模式
- 團隊模式
- 載具升級
- 角色自訂
- 更多武器模組
- 更多地圖或程序化地圖變體

## 4. UE5 高階架構

```text
FutureWorld
├─ Core
│  ├─ GameInstance
│  ├─ GameMode
│  ├─ GameState
│  ├─ PlayerController
│  ├─ PlayerState
│  └─ SaveGame
│
├─ CharacterSystem
│  ├─ CharacterBase
│  ├─ PlayerCharacter
│  ├─ AICharacter
│  ├─ CharacterData
│  ├─ CharacterAppearanceData
│  └─ CharacterAnimationSet
│
├─ ActionSystem
│  ├─ ActionComponent
│  ├─ MovementActionBase
│  ├─ WalkRunAction
│  ├─ JumpAction
│  ├─ CrouchAction
│  ├─ CrawlAction
│  ├─ EnterVehicleAction
│  └─ FutureActionModules
│
├─ VehicleSystem
│  ├─ VehicleBase
│  ├─ WheeledVehicleBase
│  ├─ CarVehicle
│  ├─ MotorcycleVehicle
│  ├─ VehicleSeatComponent
│  ├─ VehicleWeaponMountComponent
│  ├─ VehicleHealthComponent
│  └─ VehicleData
│
├─ WeaponSystem
│  ├─ WeaponBase
│  ├─ FirearmWeaponBase
│  ├─ PistolWeapon
│  ├─ RifleWeapon
│  ├─ SniperWeapon
│  ├─ WeaponModule
│  ├─ WeaponUpgrade
│  ├─ WeaponLoadout
│  └─ WeaponData
│
├─ CombatSystem
│  ├─ HealthComponent
│  ├─ DamageReceiverComponent
│  ├─ DamageSourceComponent
│  ├─ HitZoneComponent
│  ├─ TeamComponent
│  └─ CombatRuleLibrary
│
├─ AISystem
│  ├─ EnemyAIController
│  ├─ AIBehaviorTree
│  ├─ AIBlackboard
│  ├─ AICombatComponent
│  ├─ AIVehicleUseComponent
│  └─ AIDifficultyProfile
│
├─ WorldSystem
│  ├─ WorldPartitionMap
│  ├─ SpawnPoint
│  ├─ InteractableObject
│  ├─ DestructibleObject
│  ├─ Pickup
│  ├─ ObjectiveVolume
│  └─ WeatherSystem
│
├─ InputSystem
│  ├─ EnhancedInputActions
│  ├─ InputMappingContext_OnFoot
│  ├─ InputMappingContext_Vehicle
│  ├─ InputMappingContext_Menu
│  └─ GamepadReadyMappings
│
├─ CameraSystem
│  ├─ ThirdPersonCameraMode
│  ├─ FirstPersonCameraMode
│  ├─ VehicleCameraMode
│  └─ SpectatorCameraMode
│
├─ RuleSystem
│  ├─ MatchRuleSet
│  ├─ RespawnRule
│  ├─ VehicleDestructionRule
│  ├─ WinConditionRule
│  └─ GameModeDefinition
│
├─ DifficultySystem
│  ├─ DifficultyProfile
│  ├─ AIDifficultyModifier
│  ├─ DamageDifficultyModifier
│  └─ ResourceDifficultyModifier
│
└─ UI
   ├─ MainMenu
   ├─ MatchSettings
   ├─ WeaponLoadoutScreen
   ├─ HUD
   ├─ VehicleHUD
   ├─ RespawnScreen
   └─ PauseMenu
```

## 5. 核心 Unreal 類別

### 5.1 GameInstance

責任：

- 儲存全域玩家設定
- 儲存關卡載入前選定的比賽設定
- 儲存選定的武器配置
- 儲存畫面、音效與輸入偏好
- 管理存檔資料

建議類別：

- `UFWGameInstance`

### 5.2 GameMode

責任：

- 伺服器權威的比賽規則
- 生成玩家與 AI
- 套用選定的遊戲模式
- 套用勝利與失敗規則
- 管理重生流程

建議類別：

- `AFWGameModeBase`
- `AFWCompetitiveGameMode`

### 5.3 GameState

責任：

- 儲存可同步的比賽狀態
- 追蹤比賽計時
- 追蹤目前 AI 數量
- 追蹤隊伍分數
- 追蹤比賽階段

建議類別：

- `AFWGameState`

### 5.4 PlayerState

責任：

- 儲存玩家分數
- 儲存命數
- 儲存隊伍
- 儲存選定角色
- 儲存選定武器配置
- 儲存指定核心載具

建議類別：

- `AFWPlayerState`

### 5.5 PlayerController

責任：

- 輸入路由
- 攝影機切換
- UI 控制
- 角色與載具之間的 Possession 轉移

建議類別：

- `AFWPlayerController`

## 6. 角色系統

### 6.1 角色設計

角色系統需要支援卡通風格，並保留未來修改角色外觀與能力的彈性。

每個角色應以資料驅動：

- 角色 ID
- 顯示名稱
- Mesh
- 動畫組
- 移動設定
- 生命值
- 護甲值
- 可使用武器類型
- 可駕駛載具類型
- 特殊動作欄位
- 語音組
- UI 頭像

### 6.2 角色類別

建議類別：

- `AFWCharacterBase`
- `AFWPlayerCharacter`
- `AFWAICharacter`
- `UFWCharacterData`
- `UFWCharacterAppearanceData`
- `UFWMovementProfile`

### 6.3 初始角色動作

MVP 動作：

- 走路
- 跑步
- 跳躍
- 蹲下
- 爬行
- 瞄準
- 開火
- 換彈
- 互動
- 進入載具
- 離開載具

未來動作：

- 游泳
- 攀爬
- 翻越
- 滑行
- 擺盪
- 近戰攻擊
- 與 NPC 對話
- 救援隊友
- 表情動作

### 6.4 Action System 原則

玩家動作不應硬寫在角色類別中。

角色應擁有 `ActionComponent`，每個動作盡可能作為獨立模組。

這樣未來要新增游泳、擺盪或互動模式時，不需要重寫玩家角色。

## 7. 載具系統

### 7.1 載具設計

所有載具都應繼承自共同基底類別。

建議基底：

- `AFWVehicleBase`

載具資料應包含：

- 載具 ID
- 載具類型
- 顯示名稱
- 最大生命值
- 裝甲值
- 最大速度
- 加速度
- 操控性
- 煞車力
- 座位數
- 武器掛載槽
- 弱點部位
- 進入點
- 離開點
- 爆炸特效
- 殘骸 Mesh
- 重生行為

### 7.2 MVP 載具

#### 車子

定位：

- 平衡型戰術載具

特性：

- 中等速度
- 中等裝甲
- 中等操控性
- 適合道路戰鬥
- 未來可支援車載輕機槍

建議類別：

- `AFWCarVehicle`

#### 機車

定位：

- 快速突襲與撤退載具

特性：

- 高速度
- 低裝甲
- 高操控性
- 目標較小
- 風險高，但適合側襲

建議類別：

- `AFWMotorcycleVehicle`

### 7.3 未來載具

- 船
- 裝甲車
- 坦克
- 直升機
- 懸浮載具
- 無人機載具

### 7.4 進入與離開流程

預設按鍵：

- `F`

流程：

1. 玩家進入互動範圍。
2. Interactable Component 偵測最近可使用的載具座位。
3. 玩家按下 `F`。
4. PlayerController 切換 Possession 或將控制權轉交給載具。
5. 角色 Mesh 依座位設定附著或隱藏。
6. 啟用載具輸入映射。
7. 再次按下 `F`，若離開點安全，玩家離開載具。

### 7.5 核心載具規則

每位玩家或 AI 可以擁有一台核心載具。

預設規則：

- 核心載具被摧毀 = 扣一條命。

可設定替代規則：

- 核心載具被摧毀 = 直接失敗
- 核心載具被摧毀 = 計時重生
- 核心載具被摧毀 = 啟用修理目標
- 目前駕駛的任意載具被摧毀 = 懲罰
- 不使用核心載具規則，改用純分數戰鬥

## 8. 武器系統

### 8.1 武器設計理念

武器應由以下要素組成：

- 武器基底
- 射擊模式
- 彈藥類型
- 模組
- 升級等級
- 執行階段修正值

這樣可以避免每一把槍都寫成一次性的專用邏輯。

### 8.2 MVP 武器

#### 手槍

定位：

- 備用武器

特性：

- 低傷害
- 中等射速
- 低後座力
- 換彈快
- 移動中使用表現穩定

#### 步槍

定位：

- 主要戰鬥武器

特性：

- 中等傷害
- 高射速
- 中等後座力
- 中等換彈速度
- 可對步兵與輕型載具造成有效傷害

#### 狙擊槍

定位：

- 遠距離精準武器

特性：

- 高傷害
- 低射速
- 高精準度
- 對弱點傷害強
- 近距離戰鬥較弱

### 8.3 武器資料

建議欄位：

- 武器 ID
- 顯示名稱
- 武器類型
- 基礎傷害
- 對角色傷害
- 對載具傷害
- 對弱點傷害
- 射速
- 彈匣容量
- 換彈時間
- 射程
- 準確度
- 後座力
- 擴散值
- 投射物類型
- 彈藥類型
- 可用模組
- 升級樹
- 是否可步行使用
- 是否可掛載於載具

### 8.4 武器模組

初始模組分類：

- 瞄具
- 槍管
- 彈匣
- 槍托
- 彈藥類型
- 握把

模組範例：

- 紅點瞄準鏡
- 戰術瞄準鏡
- 長槍管
- 擴充彈匣
- 穿甲彈
- 穩定槍托

### 8.5 比賽前武器配置

武器自訂發生在比賽開始前。

Loadout 畫面應允許：

- 選擇主要武器
- 選擇副武器
- 選擇武器模組
- 套用升級等級
- 顯示武器數值
- 驗證配置規則

未來比賽設定可以控制：

- 開放所有升級
- 鎖定升級
- 限制武器稀有度
- 啟用隨機配置
- 啟用只能場內拾取模式

## 9. 戰鬥與傷害系統

### 9.1 傷害原則

傷害應由 Component 處理，讓角色、載具與可破壞世界物件能共用同一套傷害架構。

建議元件：

- `UFWHealthComponent`
- `UFWDamageReceiverComponent`
- `UFWDamageSourceComponent`
- `UFWHitZoneComponent`

### 9.2 傷害類型

初始傷害類型：

- 子彈
- 載具撞擊
- 爆炸
- 墜落

未來傷害類型：

- 火焰
- 電擊
- 水壓
- 毒
- 天氣
- 近戰

### 9.3 命中區域

命中區域可以強化戰術性。

角色命中區域：

- 頭部
- 身體
- 手臂
- 腿部

載具命中區域：

- 引擎
- 輪胎
- 油箱
- 駕駛座
- 武器掛點
- 車體

命中區域可套用：

- 傷害倍率
- 狀態效果
- 部位停用
- UI 回饋

### 9.4 載具毀損階段

載具生命值應支援階段式毀損：

1. 正常
2. 輕微損壞
3. 嚴重損壞
4. 癱瘓
5. 摧毀
6. 爆炸或殘骸化

這能提供更好的戰術回饋，而不是只有存活或死亡兩種狀態。

## 10. 規則系統

### 10.1 Match Rule Set

規則應資料化。

建議 `MatchRuleSet` 欄位：

- 遊戲模式
- AI 數量
- 是否啟用隊伍模式
- 比賽時間限制
- 分數限制
- 玩家命數
- 是否啟用重生
- 重生延遲
- 是否啟用核心載具
- 核心載具毀損懲罰
- 是否啟用載具修理
- 是否允許搶奪載具
- 是否啟用友軍傷害
- 武器配置規則
- 難度設定

### 10.2 MVP 預設規則

預設模式：

- 對 AI 的個人混戰競技

預設值：

- 玩家命數：3
- 啟用重生：是
- 重生延遲：5 秒
- 啟用核心載具：是
- 核心載具被摧毀：扣一條命
- 允許搶奪載具：是
- 啟用載具修理：MVP 暫不啟用
- 比賽時間限制：可選
- AI 數量：多個 AI 敵人

### 10.3 未來遊戲模式

可能模式：

- 個人混戰
- 團隊死鬥
- 核心載具獵殺
- 搶奪載具
- 車隊攻防
- 生存模式
- 開放世界沙盒
- 劇情任務
- 競速戰鬥
- 區域控制

## 11. AI 系統

### 11.1 MVP AI 目標

MVP AI 應該簡單，但足以驗證完整遊戲循環。

AI 應能：

- 巡邏
- 偵測玩家
- 移動接近玩家
- 附近有掩體時使用掩體
- 對玩家射擊
- 攻擊載具
- 在有利時進入載具
- 駕駛載具接近戰鬥
- 載具癱瘓時離開
- 規則允許時重生

### 11.2 AI 狀態

建議狀態：

- Idle
- Patrol
- Investigate
- Chase
- TakeCover
- AttackOnFoot
- SearchVehicle
- EnterVehicle
- DriveVehicle
- VehicleAttack
- Escape
- Respawn

### 11.3 AI 難度

難度應影響行為，而不是只調整血量。

難度參數：

- 反應時間
- 命中率
- 開火間隔
- 使用掩體機率
- 使用載具機率
- 瞄準弱點機率
- 側襲機率
- 撤退機率
- 攻擊性
- 資源意識

### 11.4 初始 AI 類型

MVP 類型：

- Tactical Raider

行為：

- 預設使用步槍
- 可步行攻擊
- 可進入載具
- 看見玩家核心載具時優先攻擊
- 高難度時低血量會撤退

## 12. 難度系統

### 12.1 難度等級

建議等級：

- Easy
- Normal
- Hard
- Extreme

### 12.2 難度設定檔

每個難度應定義：

- 玩家受到傷害倍率
- AI 受到傷害倍率
- AI 反應時間
- AI 命中率
- AI 載具使用傾向
- AI 弱點瞄準傾向
- 資源生成率
- 重生延遲
- 載具耐久修正

### 12.3 戰術難度方向

因為遊戲節奏偏戰術感，較高難度應讓 AI 更聰明，而不只是提高敵人血量。

好的難度變化：

- AI 更常使用掩體
- AI 會瞄準輪胎或引擎
- AI 會從多個角度攻擊
- AI 受傷時會撤退
- AI 會保護自己的核心載具

不好的難度變化：

- AI 只有更多血量
- AI 瞬間命中玩家
- AI 無視玩家必須遵守的規則

## 13. 世界系統

### 13.1 地圖風格

第一張地圖應為混合地圖。

建議區域：

- 城市區
- 郊區
- 高速道路與橋梁
- 工業區
- 開放原野
- 河流或沿海水域
- 丘陵或懸崖
- 載具車庫區
- 武器準備區

### 13.2 戰術地圖需求

地圖應支援：

- 適合狙擊槍的長視線
- 適合步槍的中距離街道
- 適合手槍與伏擊的狹窄巷弄
- 適合車子的寬道路
- 適合機車的窄路
- 作為瓶頸點的橋梁
- 掩體物件
- 載具坡道或跳台
- 通往目標的多條路線

### 13.3 世界物件

物件分類：

- 靜態場景物件
- 掩體物件
- 可破壞物件
- 爆炸物件
- 可互動物件
- 拾取物
- 出生點
- 載具出生點
- AI 巡邏點
- 目標區域

### 13.4 Interactable Object Interface

所有可互動物件應實作共同介面。

建議介面：

- `IFWInteractable`

函式：

- `CanInteract`
- `GetInteractionText`
- `BeginInteract`
- `CancelInteract`
- `CompleteInteract`

這讓 `F` 鍵可以統一支援載具、門、拾取物、終端機與未來 NPC。

## 14. 攝影機系統

### 14.1 預設攝影機

預設：

- 第三人稱越肩視角

需求：

- 清楚看見角色
- 瞄準閱讀性良好
- 移動平順
- 處理攝影機碰撞
- 載具切換舒適

### 14.2 攝影機切換

玩家可以切換攝影機模式。

初始模式：

- 步行第三人稱
- 步行第一人稱
- 載具第三人稱

未來模式：

- 載具第一人稱
- 觀戰視角
- 分割螢幕玩家攝影機
- 無人機攝影機

## 15. 輸入系統

### 15.1 輸入平台

MVP 平台：

- PC 鍵盤滑鼠

未來：

- Gamepad
- 如果平台規劃需要，可支援 Switch 控制器

### 15.2 Enhanced Input Mapping Contexts

建議 Context：

- OnFoot
- Vehicle
- Menu
- Debug
- Spectator

### 15.3 預設控制

建議鍵盤滑鼠控制：

- WASD：移動
- Mouse：視角
- Left mouse：開火
- Right mouse：瞄準
- R：換彈
- Space：跳躍
- Shift：跑步
- Ctrl：蹲下
- C：切換爬行
- F：互動 / 進入載具 / 離開載具
- 1/2/3：武器欄位
- V：切換攝影機
- Esc：暫停

## 16. UI 系統

### 16.1 主選單

MVP 選單項目：

- 開始比賽
- 武器配置
- 比賽設定
- 選項
- 離開

### 16.2 比賽設定

設定項目：

- AI 數量
- 難度
- 玩家命數
- 是否啟用重生
- 重生延遲
- 是否啟用核心載具
- 核心載具毀損結果
- 是否允許搶奪載具
- 比賽時間限制

### 16.3 武器配置畫面

功能：

- 選擇手槍 / 步槍 / 狙擊槍
- 裝備模組
- 檢視數值變化
- 儲存配置

### 16.4 HUD

HUD 應顯示：

- 玩家生命值
- 護甲
- 目前武器
- 彈藥
- 準星
- 命數
- 核心載具生命值
- 駕駛時的載具生命值
- 小地圖佔位
- AI 警戒提示

## 17. 美術方向

### 17.1 視覺風格

風格：

- 卡通
- 清楚的輪廓
- 易讀的戰鬥形狀
- 誇張化載具
- 色彩豐富但保持戰術可讀性

### 17.2 角色風格

角色應使用：

- 風格化比例
- 強烈輪廓
- 易讀的隊伍色彩
- 模組化服裝
- 清楚的動畫姿勢

### 17.3 載具風格

載具應該：

- 稍微誇張化
- 遠距離也容易辨識
- 透過階段變化清楚呈現損壞
- 能相容武器掛點

### 17.4 環境風格

世界應該：

- 亮度足以保持可讀性
- 混合城市與自然區域
- 為載具與步兵戰鬥而設計
- 避免過度寫實雜物影響玩法清晰度

## 18. 預留多人連線設計

### 18.1 第一版

第一版是單人對 AI。

但架構必須為未來多人連線做準備。

### 18.2 Network-Ready 規則

早期系統應避免：

- 只有客戶端能決定傷害
- 硬寫只支援單一玩家
- 直接依賴唯一全域玩家角色
- UI 邏輯控制遊戲狀態
- 無法同步的隨機玩法事件

### 18.3 未來線上多人需求

未來線上多人需要：

- 同步玩家狀態
- 同步載具狀態
- 同步武器開火
- 伺服器權威傷害
- 射擊延遲補償
- 載具移動修正
- 配對或房間系統
- 隊伍分配
- 加入與離開流程
- 反作弊考量

### 18.4 單機分割螢幕需求

未來單機分割螢幕需要：

- 多個本機 PlayerController
- 多個攝影機
- 多個 HUD 實例
- 每位玩家各自的輸入裝置
- 分割螢幕 UI 縮放
- 本機玩家出生處理

## 19. 未來天氣系統設計

### 19.1 天氣目標

天氣應同時影響視覺與玩法。

可能天氣：

- 晴天
- 雨天
- 霧天
- 暴風雨
- 夜晚
- 強風

### 19.2 玩法效果

範例：

- 雨天降低載具操控性
- 霧天降低遠距離能見度
- 暴風雨影響船與飛行載具
- 夜晚提高燈光與偵測工具的重要性
- 進階模式中，風會影響投射物

### 19.3 技術方向

建議系統：

- `UFWWeatherSubsystem`
- Weather profile data assets
- 世界光照控制
- Material Parameter Collections
- Niagara 特效
- 音效層
- 玩法修正值

## 20. Data Asset 規劃

### 20.1 建議 Data Assets

為以下內容建立 Data Asset：

- 角色資料
- 角色外觀資料
- 移動設定
- 載具資料
- 武器資料
- 武器模組資料
- 升級資料
- AI 難度設定
- 比賽規則集
- 遊戲模式定義
- 天氣設定

### 20.2 好處

Data Asset 可以：

- 更快調整數值
- 讓 Blueprint 設定更乾淨
- 減少硬寫 C++
- 讓未來內容擴充更容易
- 提供對設計者友善的工作流程

## 21. 建議 C++ 命名

專案前綴：

- `FW`

範例：

- `AFWPlayerCharacter`
- `AFWVehicleBase`
- `UFWWeaponData`
- `UFWHealthComponent`
- `AFWGameModeBase`
- `UFWMatchRuleSet`
- `UFWActionComponent`

資料夾命名：

```text
Source/FutureWorld/Core
Source/FutureWorld/Characters
Source/FutureWorld/Actions
Source/FutureWorld/Vehicles
Source/FutureWorld/Weapons
Source/FutureWorld/Combat
Source/FutureWorld/AI
Source/FutureWorld/World
Source/FutureWorld/Rules
Source/FutureWorld/UI
```

## 22. 開發路線

### 22.1 Phase 0：專案建立

目標：

- 建立 UE5 C++ 專案
- 啟用 Enhanced Input
- 建立專案資料夾結構
- 建立基底 GameMode、GameState、PlayerState、PlayerController
- 建立初始測試地圖

### 22.2 Phase 1：玩家原型

目標：

- 第三人稱角色
- 走路 / 跑步 / 跳躍 / 蹲下 / 爬行
- 攝影機切換
- 基礎 HUD
- 互動鍵框架

### 22.3 Phase 2：武器原型

目標：

- 武器基底類別
- 手槍、步槍、狙擊槍
- 彈藥與換彈
- 傷害元件
- 基礎命中回饋

### 22.4 Phase 3：載具原型

目標：

- 載具基底類別
- 車子
- 機車
- 使用 `F` 進入 / 離開
- 載具生命值
- 載具毀損

### 22.5 Phase 4：AI 原型

目標：

- AI Controller
- Behavior Tree
- 巡邏 / 追擊 / 攻擊
- AI 使用武器
- AI 使用載具原型

### 22.6 Phase 5：規則系統

目標：

- 比賽設定
- 命數
- 重生
- 核心載具規則
- 勝利 / 失敗流程

### 22.7 Phase 6：世界 MVP

目標：

- 建立混合地圖 Blockout
- 加入道路與戰鬥區
- 加入出生點
- 加入掩體
- 加入測試載具路線

### 22.8 Phase 7：武器配置

目標：

- 比賽前武器選擇
- 武器模組
- 升級資料
- 數值預覽

### 22.9 Phase 8：MVP 打磨

目標：

- 卡通美術初步整理
- UI 改善
- 特效
- 音效佔位
- 難度設定
- 錯誤修正
- 效能檢查

## 23. 第一個可玩版本目標

當以下條件達成時，第一個可玩版本即完成：

- 玩家可以出生在開放世界中。
- 玩家可以移動、跳躍、蹲下、爬行、瞄準與射擊。
- 玩家可以用 `F` 進入與離開車子或機車。
- 玩家可以傷害 AI 與載具。
- AI 敵人可以攻擊玩家。
- 至少一個 AI 可以使用或追逐載具。
- 玩家與 AI 有命數。
- 摧毀核心載具會扣一條命。
- 比賽可以結束。
- 可以在比賽前選擇武器配置。
- 比賽設定可以改變基本規則。

## 24. 待確認設計問題

正式量產前應回答：

1. 玩家是否擁有一台固定核心載具，或核心載具可在比賽中重新指定？
2. AI 是否與玩家使用相同核心載具規則？
3. 載具修理應放在 MVP，還是 MVP 後再做？
4. 載具是否需要燃料或能量？
5. 武器應使用 hitscan、投射物，或混合行為？
6. 第一張地圖應是連續島嶼 / 城市，還是有水域邊界的大陸區？
7. MVP 是否支援隊伍，或只做個人混戰？
8. 武器升級應透過進度解鎖，還是在比賽設定中全部開放？
9. 難度只影響 AI，還是也影響資源與載具耐久？
10. MVP 的目標 PC 硬體等級為何？

## 25. 建議立即下一步

1. 建立 Unreal Engine 5 C++ 專案。
2. 設定專案前綴與資料夾結構。
3. 實作基礎框架類別。
4. 先建立小型測試地圖，再做大型世界。
5. 原型化玩家移動與攝影機切換。
6. 原型化使用 `F` 的互動系統。
7. 加入一把測試武器與一台測試載具。
8. 加入一個 AI 敵人。
9. 驗證完整循環：
   - 出生
   - 移動
   - 射擊
   - 進入載具
   - 傷害載具
   - 扣命
   - 重生
   - 比賽結束

