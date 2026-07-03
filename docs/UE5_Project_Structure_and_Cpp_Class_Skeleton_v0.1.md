# UE5 Project Structure and C++ Class Skeleton v0.1

## 1. 文件目的

這份文件定義本遊戲在 Unreal Engine 5 中的專案結構、C++ 類別骨架、資料資產規劃與模組邊界。

目標不是一次寫完所有功能，而是先建立一個可以長期擴充的骨架，讓後續新增載具、武器、AI、多人連線、分割螢幕、天氣系統時，不需要重寫核心架構。

## 2. 建議專案基本資訊

專案顯示名稱：

- `Future World`

專案縮寫：

- `FW`

C++ 類別前綴：

- `FW`

主要模組名稱：

- `FW`

建議建立方式：

1. 使用 Unreal Engine 5 建立 C++ 專案。
2. Template 可先選 `Third Person`。
3. Target Platform 先選 Desktop。
4. Quality Preset 可先選 Maximum 或 Scalable，依硬體調整。
5. 建立後再逐步加入本文件的 C++ 類別與資料資產。

## 3. C++ 與 Blueprint 分工

### 3.1 C++ 負責

- 核心資料結構
- 玩家、載具、武器、傷害、規則、AI 的基底類別
- 可重用 Component
- 伺服器權威邏輯
- 未來多人同步需要的資料
- 效能敏感邏輯

### 3.2 Blueprint 負責

- 角色外觀配置
- 武器數值調整
- 載具數值調整
- UI 串接
- 關卡放置
- AI Behavior Tree 細部流程
- 特效、音效、動畫事件

### 3.3 原則

核心規則放 C++，內容調整放 Blueprint/Data Asset。

不要把勝敗規則、傷害計算、生命扣除、載具歸屬等重要邏輯只寫在 Widget 或關卡 Blueprint 裡。

## 4. 建議資料夾結構

### 4.1 Source 結構

```text
Source/
└─ FW/
   ├─ FW.Build.cs
   ├─ FW.cpp
   ├─ FW.h
   │
   ├─ Core/
   │  ├─ FWGameInstance.h
   │  ├─ FWGameModeBase.h
   │  ├─ FWCompetitiveGameMode.h
   │  ├─ FWGameState.h
   │  ├─ FWPlayerController.h
   │  ├─ FWPlayerState.h
   │  └─ FWSaveGame.h
   │
   ├─ Characters/
   │  ├─ FWCharacterBase.h
   │  ├─ FWPlayerCharacter.h
   │  ├─ FWAICharacter.h
   │  ├─ FWCharacterData.h
   │  ├─ FWCharacterAppearanceData.h
   │  └─ FWMovementProfile.h
   │
   ├─ Actions/
   │  ├─ FWActionComponent.h
   │  ├─ FWActionBase.h
   │  ├─ FWMovementActionBase.h
   │  ├─ FWEnterVehicleAction.h
   │  └─ FWActionTypes.h
   │
   ├─ Vehicles/
   │  ├─ FWVehicleBase.h
   │  ├─ FWWheeledVehicleBase.h
   │  ├─ FWCarVehicle.h
   │  ├─ FWMotorcycleVehicle.h
   │  ├─ FWVehicleData.h
   │  ├─ FWVehicleSeatComponent.h
   │  └─ FWVehicleWeaponMountComponent.h
   │
   ├─ Weapons/
   │  ├─ FWWeaponBase.h
   │  ├─ FWFirearmWeaponBase.h
   │  ├─ FWPistolWeapon.h
   │  ├─ FWRifleWeapon.h
   │  ├─ FWSniperWeapon.h
   │  ├─ FWWeaponData.h
   │  ├─ FWWeaponModuleData.h
   │  └─ FWWeaponLoadout.h
   │
   ├─ Combat/
   │  ├─ FWHealthComponent.h
   │  ├─ FWDamageReceiverComponent.h
   │  ├─ FWDamageSourceComponent.h
   │  ├─ FWHitZoneComponent.h
   │  ├─ FWTeamComponent.h
   │  └─ FWCombatTypes.h
   │
   ├─ AI/
   │  ├─ FWEnemyAIController.h
   │  ├─ FWAICombatComponent.h
   │  ├─ FWAIVehicleUseComponent.h
   │  └─ FWAIDifficultyProfile.h
   │
   ├─ World/
   │  ├─ FWInteractable.h
   │  ├─ FWInteractableComponent.h
   │  ├─ FWSpawnPoint.h
   │  ├─ FWVehicleSpawnPoint.h
   │  ├─ FWDestructibleObject.h
   │  └─ FWWeatherProfile.h
   │
   ├─ Rules/
   │  ├─ FWMatchRuleSet.h
   │  ├─ FWGameModeDefinition.h
   │  ├─ FWRespawnRule.h
   │  └─ FWWinConditionRule.h
   │
   └─ UI/
      ├─ FWHUDWidget.h
      ├─ FWVehicleHUDWidget.h
      ├─ FWMatchSettingsWidget.h
      └─ FWWeaponLoadoutWidget.h
```

### 4.2 Content 結構

```text
Content/
├─ FW/
│  ├─ Characters/
│  │  ├─ Blueprints/
│  │  ├─ Meshes/
│  │  ├─ Animations/
│  │  └─ Data/
│  │
│  ├─ Vehicles/
│  │  ├─ Blueprints/
│  │  ├─ Meshes/
│  │  ├─ Materials/
│  │  └─ Data/
│  │
│  ├─ Weapons/
│  │  ├─ Blueprints/
│  │  ├─ Meshes/
│  │  ├─ VFX/
│  │  ├─ SFX/
│  │  └─ Data/
│  │
│  ├─ AI/
│  │  ├─ BehaviorTrees/
│  │  ├─ Blackboards/
│  │  └─ Data/
│  │
│  ├─ Maps/
│  │  ├─ Test/
│  │  └─ OpenWorld/
│  │
│  ├─ UI/
│  │  ├─ Widgets/
│  │  └─ Textures/
│  │
│  ├─ Input/
│  │  ├─ Actions/
│  │  └─ MappingContexts/
│  │
│  ├─ Rules/
│  │  └─ Data/
│  │
│  ├─ Materials/
│  ├─ VFX/
│  ├─ SFX/
│  └─ Dev/
```

## 5. Build.cs 建議依賴

初期建議依賴：

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core",
    "CoreUObject",
    "Engine",
    "InputCore",
    "EnhancedInput",
    "UMG",
    "AIModule",
    "GameplayTasks",
    "NavigationSystem"
});
```

若正式使用 Gameplay Ability System，再加入：

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "GameplayAbilities",
    "GameplayTags",
    "GameplayTasks"
});
```

建議策略：

- MVP 先用自訂 Action/Combat Component 建立清楚骨架。
- 等武器、狀態效果、技能、升級需求穩定後，再決定是否全面導入 Gameplay Ability System。

## 6. Core 類別骨架

### 6.1 `UFWGameInstance`

責任：

- 保存全域設定
- 保存玩家選擇的 Match Rule Set
- 保存武器 Loadout
- 保存輸入、畫面、音效偏好
- 未來管理登入、房間、線上服務

骨架：

```cpp
UCLASS()
class FW_API UFWGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FW|Rules")
    TObjectPtr<class UFWMatchRuleSet> SelectedRuleSet;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="FW|Loadout")
    TObjectPtr<class UFWWeaponLoadout> SelectedLoadout;
};
```

### 6.2 `AFWCompetitiveGameMode`

責任：

- 比賽開始與結束
- AI 生成
- 玩家重生
- 生命數管理
- 核心載具毀損懲罰

關鍵函式：

```cpp
UCLASS()
class FW_API AFWCompetitiveGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="FW|Match")
    void StartMatchFlow();

    UFUNCTION(BlueprintCallable, Category="FW|Match")
    void EndMatchFlow();

    UFUNCTION(BlueprintCallable, Category="FW|Respawn")
    void RequestRespawn(AController* Controller);

    UFUNCTION(BlueprintCallable, Category="FW|Vehicles")
    void HandleCoreVehicleDestroyed(AController* OwnerController);
};
```

### 6.3 `AFWPlayerState`

責任：

- 玩家分數
- 玩家命數
- 隊伍
- 核心載具
- 武器配置

骨架：

```cpp
UCLASS()
class FW_API AFWPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Replicated, Category="FW|Match")
    int32 Lives = 3;

    UPROPERTY(BlueprintReadOnly, Replicated, Category="FW|Team")
    int32 TeamId = 0;

    UPROPERTY(BlueprintReadOnly, Replicated, Category="FW|Vehicle")
    TObjectPtr<class AFWVehicleBase> CoreVehicle;
};
```

## 7. Character 類別骨架

### 7.1 `AFWCharacterBase`

責任：

- 角色共同基底
- 掛載生命、隊伍、武器、Action Component
- 支援玩家與 AI 共用邏輯

```cpp
UCLASS()
class FW_API AFWCharacterBase : public ACharacter
{
    GENERATED_BODY()

public:
    AFWCharacterBase();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="FW|Components")
    TObjectPtr<class UFWHealthComponent> HealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="FW|Components")
    TObjectPtr<class UFWTeamComponent> TeamComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="FW|Components")
    TObjectPtr<class UFWActionComponent> ActionComponent;

    UPROPERTY(BlueprintReadOnly, Category="FW|Weapon")
    TObjectPtr<class AFWWeaponBase> CurrentWeapon;
};
```

### 7.2 `AFWPlayerCharacter`

責任：

- 玩家角色控制
- Input 綁定
- 攝影機切換
- 互動偵測

```cpp
UCLASS()
class FW_API AFWPlayerCharacter : public AFWCharacterBase
{
    GENERATED_BODY()

public:
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

protected:
    UFUNCTION()
    void Move(const struct FInputActionValue& Value);

    UFUNCTION()
    void Look(const struct FInputActionValue& Value);

    UFUNCTION()
    void Interact();

    UFUNCTION()
    void ToggleCameraMode();
};
```

## 8. Action System 類別骨架

### 8.1 設計目的

玩家行為不要全部塞進 Character。

`UFWActionComponent` 負責管理目前角色可使用的行動模組，例如：

- 跑步
- 跳躍
- 蹲下
- 爬行
- 上下載具
- 未來游泳
- 未來攀爬
- 未來蜘蛛人式擺盪

### 8.2 `UFWActionBase`

```cpp
UCLASS(Blueprintable, Abstract)
class FW_API UFWActionBase : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, Category="FW|Action")
    bool CanStartAction(AActor* InstigatorActor) const;

    UFUNCTION(BlueprintNativeEvent, Category="FW|Action")
    void StartAction(AActor* InstigatorActor);

    UFUNCTION(BlueprintNativeEvent, Category="FW|Action")
    void StopAction(AActor* InstigatorActor);
};
```

### 8.3 `UFWActionComponent`

```cpp
UCLASS(ClassGroup=(FW), meta=(BlueprintSpawnableComponent))
class FW_API UFWActionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="FW|Action")
    bool TryStartActionByName(FName ActionName);

    UFUNCTION(BlueprintCallable, Category="FW|Action")
    void StopActionByName(FName ActionName);

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Action")
    TMap<FName, TSubclassOf<UFWActionBase>> AvailableActions;
};
```

## 9. Vehicle 類別骨架

### 9.1 `AFWVehicleBase`

責任：

- 載具共同基底
- 生命值
- 座位
- 武器掛點
- 進出控制
- 核心載具歸屬
- 未來多人同步

```cpp
UCLASS()
class FW_API AFWVehicleBase : public APawn
{
    GENERATED_BODY()

public:
    AFWVehicleBase();

    UFUNCTION(BlueprintCallable, Category="FW|Vehicle")
    bool CanEnterVehicle(AController* Controller) const;

    UFUNCTION(BlueprintCallable, Category="FW|Vehicle")
    bool EnterVehicle(AController* Controller);

    UFUNCTION(BlueprintCallable, Category="FW|Vehicle")
    bool ExitVehicle(AController* Controller);

    UFUNCTION(BlueprintCallable, Category="FW|Vehicle")
    void HandleVehicleDestroyed();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Vehicle")
    TObjectPtr<class UFWVehicleData> VehicleData;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="FW|Components")
    TObjectPtr<class UFWHealthComponent> HealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="FW|Components")
    TArray<TObjectPtr<class UFWVehicleSeatComponent>> Seats;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="FW|Components")
    TArray<TObjectPtr<class UFWVehicleWeaponMountComponent>> WeaponMounts;
};
```

### 9.2 `UFWVehicleData`

```cpp
UCLASS(BlueprintType)
class FW_API UFWVehicleData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Vehicle")
    FName VehicleId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Vehicle")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Stats")
    float MaxHealth = 1000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Stats")
    float Armor = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Stats")
    float MaxSpeed = 1200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Stats")
    int32 SeatCount = 1;
};
```

## 10. Weapon 類別骨架

### 10.1 `AFWWeaponBase`

責任：

- 武器共同基底
- 裝備/卸下
- 開火/停止開火
- 換彈
- 套用模組與升級

```cpp
UCLASS()
class FW_API AFWWeaponBase : public AActor
{
    GENERATED_BODY()

public:
    AFWWeaponBase();

    UFUNCTION(BlueprintCallable, Category="FW|Weapon")
    virtual void Equip(AActor* NewOwner);

    UFUNCTION(BlueprintCallable, Category="FW|Weapon")
    virtual void Unequip();

    UFUNCTION(BlueprintCallable, Category="FW|Weapon")
    virtual void StartFire();

    UFUNCTION(BlueprintCallable, Category="FW|Weapon")
    virtual void StopFire();

    UFUNCTION(BlueprintCallable, Category="FW|Weapon")
    virtual void Reload();

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Weapon")
    TObjectPtr<class UFWWeaponData> WeaponData;
};
```

### 10.2 `UFWWeaponData`

```cpp
UENUM(BlueprintType)
enum class EFWWeaponType : uint8
{
    Pistol,
    Rifle,
    Sniper,
    VehicleMounted,
    Future
};

UCLASS(BlueprintType)
class FW_API UFWWeaponData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Weapon")
    FName WeaponId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Weapon")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Weapon")
    EFWWeaponType WeaponType = EFWWeaponType::Pistol;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Stats")
    float BaseDamage = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Stats")
    float VehicleDamageMultiplier = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Stats")
    float FireRate = 3.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Stats")
    int32 MagazineSize = 12;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Stats")
    float ReloadTime = 1.5f;
};
```

### 10.3 `UFWWeaponLoadout`

```cpp
UCLASS(BlueprintType)
class FW_API UFWWeaponLoadout : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Loadout")
    TObjectPtr<UFWWeaponData> PrimaryWeapon;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Loadout")
    TObjectPtr<UFWWeaponData> SecondaryWeapon;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Loadout")
    TArray<TObjectPtr<class UFWWeaponModuleData>> EquippedModules;
};
```

## 11. Combat 類別骨架

### 11.1 `UFWHealthComponent`

責任：

- 生命值
- 死亡事件
- 受傷事件
- 支援角色、載具、可破壞物共用

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFWDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFWHealthChanged, float, CurrentHealth, float, MaxHealth);

UCLASS(ClassGroup=(FW), meta=(BlueprintSpawnableComponent))
class FW_API UFWHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFWHealthComponent();

    UFUNCTION(BlueprintCallable, Category="FW|Health")
    void ApplyDamage(float DamageAmount, AActor* DamageInstigator);

    UFUNCTION(BlueprintCallable, Category="FW|Health")
    bool IsDead() const;

    UPROPERTY(BlueprintAssignable, Category="FW|Health")
    FOnFWDeath OnDeath;

    UPROPERTY(BlueprintAssignable, Category="FW|Health")
    FOnFWHealthChanged OnHealthChanged;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Health")
    float MaxHealth = 100.0f;

    UPROPERTY(BlueprintReadOnly, Category="FW|Health")
    float CurrentHealth = 100.0f;
};
```

### 11.2 `UFWHitZoneComponent`

責任：

- 定義弱點
- 提供傷害倍率
- 支援載具部位破壞

```cpp
UCLASS(ClassGroup=(FW), meta=(BlueprintSpawnableComponent))
class FW_API UFWHitZoneComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="FW|HitZone")
    float GetDamageMultiplier(FName HitZoneName) const;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|HitZone")
    TMap<FName, float> DamageMultipliers;
};
```

## 12. Rule 類別骨架

### 12.1 `UFWMatchRuleSet`

```cpp
UENUM(BlueprintType)
enum class EFWCoreVehiclePenalty : uint8
{
    LoseLife,
    InstantDefeat,
    TimedRespawn,
    RepairObjective,
    None
};

UCLASS(BlueprintType)
class FW_API UFWMatchRuleSet : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Rules")
    int32 AIEnemyCount = 5;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Rules")
    int32 PlayerLives = 3;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Rules")
    bool bRespawnEnabled = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Rules")
    float RespawnDelay = 5.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Rules")
    bool bCoreVehicleEnabled = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Rules")
    EFWCoreVehiclePenalty CoreVehiclePenalty = EFWCoreVehiclePenalty::LoseLife;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|Rules")
    bool bVehicleStealingEnabled = true;
};
```

## 13. AI 類別骨架

### 13.1 `AFWEnemyAIController`

責任：

- 控制 AI 角色
- 啟動 Behavior Tree
- 讀取難度設定
- 協調步行與載具行為

```cpp
UCLASS()
class FW_API AFWEnemyAIController : public AAIController
{
    GENERATED_BODY()

public:
    virtual void OnPossess(APawn* InPawn) override;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|AI")
    TObjectPtr<class UBehaviorTree> BehaviorTree;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|AI")
    TObjectPtr<class UFWAIDifficultyProfile> DifficultyProfile;
};
```

### 13.2 `UFWAIDifficultyProfile`

```cpp
UCLASS(BlueprintType)
class FW_API UFWAIDifficultyProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|AI")
    float ReactionTime = 0.8f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|AI")
    float Accuracy = 0.5f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|AI")
    float VehicleUsageChance = 0.35f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|AI")
    float WeakPointTargetChance = 0.15f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FW|AI")
    float CoverUsageChance = 0.4f;
};
```

## 14. World 類別骨架

### 14.1 `IFWInteractable`

責任：

- 讓 F 鍵可統一互動
- 支援載具、門、道具、NPC、終端機

```cpp
UINTERFACE(BlueprintType)
class FW_API UFWInteractable : public UInterface
{
    GENERATED_BODY()
};

class FW_API IFWInteractable
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="FW|Interaction")
    bool CanInteract(AActor* Interactor) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="FW|Interaction")
    FText GetInteractionText(AActor* Interactor) const;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="FW|Interaction")
    void Interact(AActor* Interactor);
};
```

### 14.2 `AFWSpawnPoint`

```cpp
UCLASS()
class FW_API AFWSpawnPoint : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FW|Spawn")
    FName SpawnTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="FW|Spawn")
    int32 TeamId = 0;
};
```

## 15. Input Asset 規劃

### 15.1 Input Actions

建議建立：

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
- `IA_EnterExitVehicle`
- `IA_SwitchWeapon`
- `IA_ToggleCamera`
- `IA_Pause`

### 15.2 Mapping Contexts

建議建立：

- `IMC_OnFoot`
- `IMC_Vehicle`
- `IMC_Menu`
- `IMC_Debug`

切換原則：

- 角色步行時啟用 `IMC_OnFoot`
- 進入載具後啟用 `IMC_Vehicle`
- 打開選單時啟用 `IMC_Menu`

## 16. Data Asset 建立清單

MVP 建議先建立這些資料資產：

### 16.1 Weapons

- `DA_Weapon_Pistol`
- `DA_Weapon_Rifle`
- `DA_Weapon_Sniper`
- `DA_Module_RedDot`
- `DA_Module_ExtendedMag`
- `DA_Module_LongBarrel`
- `DA_Loadout_Default`

### 16.2 Vehicles

- `DA_Vehicle_Car_Balanced`
- `DA_Vehicle_Motorcycle_Fast`

### 16.3 Characters

- `DA_Character_Player_Default`
- `DA_Character_AI_TacticalRaider`

### 16.4 Rules

- `DA_RuleSet_DefaultCompetitive`
- `DA_RuleSet_NoRespawn`
- `DA_RuleSet_CoreVehicleInstantDefeat`

### 16.5 Difficulty

- `DA_Difficulty_Easy`
- `DA_Difficulty_Normal`
- `DA_Difficulty_Hard`
- `DA_Difficulty_Extreme`

## 17. Blueprint 建立清單

MVP 建議先建立：

### 17.1 Characters

- `BP_PlayerCharacter`
- `BP_AI_TacticalRaider`

### 17.2 Vehicles

- `BP_Vehicle_Car`
- `BP_Vehicle_Motorcycle`

### 17.3 Weapons

- `BP_Weapon_Pistol`
- `BP_Weapon_Rifle`
- `BP_Weapon_Sniper`

### 17.4 Game Framework

- `BP_CompetitiveGameMode`
- `BP_PlayerController`
- `BP_GameState`
- `BP_PlayerState`

### 17.5 UI

- `WBP_MainMenu`
- `WBP_MatchSettings`
- `WBP_WeaponLoadout`
- `WBP_HUD`
- `WBP_RespawnScreen`

## 18. 第一階段實作順序

建議順序：

1. 建立 UE5 C++ Third Person 專案。
2. 加入 Enhanced Input。
3. 建立 Source 資料夾結構。
4. 建立 Core 類別。
5. 建立 HealthComponent。
6. 建立 Interactable Interface。
7. 建立 PlayerCharacter 與基本移動。
8. 建立 WeaponBase 和一把 Pistol。
9. 建立 VehicleBase 和一台 Car。
10. 建立 F 鍵互動上下載具。
11. 建立 AI Character 和 AI Controller。
12. 建立 MatchRuleSet。
13. 建立核心載具扣命流程。
14. 建立簡單 HUD。
15. 打通完整遊戲循環。

## 19. 第一個測試場景

測試地圖先不要直接做完整大地圖。

建議建立：

- `L_Test_CombatGarage`

內容：

- 一小塊平地
- 幾個掩體
- 一台車
- 一台機車
- 三把武器測試點
- 一個玩家出生點
- 三個 AI 出生點
- 一段直線道路
- 一個簡單坡道

測試目標：

- 玩家移動是否舒服
- 武器射擊是否可用
- AI 是否會追擊
- 車輛是否能進出
- 車輛生命是否會扣
- 核心載具毀損是否扣命
- 重生流程是否順

## 20. 開發風險提醒

### 20.1 不要太早做完整大地圖

先做測試場景，把角色、武器、載具、AI、規則循環打通。

大地圖應該等核心玩法可玩後再擴大。

### 20.2 不要太早堆太多武器

先把三把武器的基底做穩。

手槍、步槍、狙擊槍如果共用架構清楚，未來新增武器才會快。

### 20.3 不要把 AI 一開始做得太聰明

AI 先能完成：

- 看見玩家
- 追擊玩家
- 攻擊玩家
- 使用載具

之後再加掩體、包抄、保護核心載具。

### 20.4 不要忽略多人連線預留

就算 MVP 是單機，也要避免：

- 全部邏輯只寫在本機 UI
- 假設世界只有一個玩家
- 傷害由客戶端任意決定
- AI 或載具狀態無法同步

## 21. 下一份文件建議

下一步可以建立：

- `UE5_MVP_Implementation_Tasks_v0.1.md`

內容會把開發拆成可執行任務：

- Phase 0 專案建立
- Phase 1 玩家控制
- Phase 2 武器
- Phase 3 載具
- Phase 4 AI
- Phase 5 規則與 UI
- Phase 6 第一個可玩版本驗收
