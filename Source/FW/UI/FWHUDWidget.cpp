#include "FWHUDWidget.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void UFWHUDWidget::SetHUDState(const FFWHUDStateSnapshot& NewHUDState)
{
	HUDState = NewHUDState;
	UpdateHUDText();
	OnHUDStateChanged();
}

TSharedRef<SWidget> UFWHUDWidget::RebuildWidget()
{
	const FSlateFontInfo HeaderFont = FCoreStyle::GetDefaultFontStyle("Bold", 18);
	const FSlateFontInfo BodyFont = FCoreStyle::GetDefaultFontStyle("Regular", 14);

	TSharedRef<SWidget> RootWidget =
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(FMargin(16.0f))
		[
			SNew(SBox)
			.WidthOverride(360.0f)
			[
				SNew(SBorder)
				.Padding(FMargin(12.0f))
				.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.72f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("FW Combat Garage")))
						.Font(HeaderFont)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.94f, 0.9f, 1.0f)))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(HealthText, STextBlock)
						.Font(BodyFont)
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(LivesText, STextBlock)
						.Font(BodyFont)
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(WeaponText, STextBlock)
						.Font(BodyFont)
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(VehicleText, STextBlock)
						.Font(BodyFont)
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SAssignNew(CoreVehicleText, STextBlock)
						.Font(BodyFont)
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SAssignNew(StateText, STextBlock)
						.Font(BodyFont)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.68f, 0.86f, 1.0f, 1.0f)))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SAssignNew(MovementText, STextBlock)
						.Font(BodyFont)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.72f, 1.0f, 0.78f, 1.0f)))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SAssignNew(AIText, STextBlock)
						.Font(BodyFont)
						.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.84f, 0.52f, 1.0f)))
					]
				]
			]
		];

	UpdateHUDText();
	return RootWidget;
}

void UFWHUDWidget::UpdateHUDText()
{
	if (HealthText)
	{
		HealthText->SetText(FormatHealthLine(TEXT("Player"), HUDState.PlayerHealth, HUDState.PlayerMaxHealth));
	}
	if (LivesText)
	{
		LivesText->SetText(FText::Format(NSLOCTEXT("FW", "HUDLives", "Lives: {0}"), FText::AsNumber(HUDState.Lives)));
	}
	if (WeaponText)
	{
		const FString WeaponName = HUDState.WeaponContentId.IsNone() ? TEXT("None") : HUDState.WeaponContentId.ToString();
		WeaponText->SetText(FText::Format(NSLOCTEXT("FW", "HUDWeapon", "Weapon: {0} | Ammo: {1}"), FText::FromString(WeaponName), FText::AsNumber(HUDState.CurrentAmmo)));
	}
	if (VehicleText)
	{
		VehicleText->SetText(FormatHealthLine(TEXT("Vehicle"), HUDState.VehicleHealth, HUDState.VehicleMaxHealth));
	}
	if (CoreVehicleText)
	{
		CoreVehicleText->SetText(FormatHealthLine(TEXT("Core Vehicle"), HUDState.CoreVehicleHealth, HUDState.CoreVehicleMaxHealth));
	}
	if (StateText)
	{
		const FString PlayerState = HUDState.PlayerStateTag.IsValid() ? HUDState.PlayerStateTag.ToString() : TEXT("None");
		const FString MatchState = HUDState.MatchStateTag.IsValid() ? HUDState.MatchStateTag.ToString() : TEXT("None");
		StateText->SetText(FText::Format(NSLOCTEXT("FW", "HUDStates", "State: {0}\nMatch: {1}"), FText::FromString(PlayerState), FText::FromString(MatchState)));
	}
	if (MovementText)
	{
		MovementText->SetText(FText::Format(
			NSLOCTEXT("FW", "HUDMovementWithInput", "Loc: {0}, {1}, {2}\nSpeed: {3}/{4}\nFalling: {5} | Crouched: {6} | Crawling: {7}\nInput: {8}\nCamera: {9}\nInteract: {10}"),
			FText::AsNumber(FMath::RoundToInt(HUDState.PlayerLocation.X)),
			FText::AsNumber(FMath::RoundToInt(HUDState.PlayerLocation.Y)),
			FText::AsNumber(FMath::RoundToInt(HUDState.PlayerLocation.Z)),
			FText::AsNumber(FMath::RoundToInt(HUDState.PlayerSpeed)),
			FText::AsNumber(FMath::RoundToInt(HUDState.PlayerMaxWalkSpeed)),
			FText::FromString(HUDState.bPlayerFalling ? TEXT("Yes") : TEXT("No")),
			FText::FromString(HUDState.bPlayerCrouched ? TEXT("Yes") : TEXT("No")),
			FText::FromString(HUDState.bPlayerCrawling ? TEXT("Yes") : TEXT("No")),
			FText::FromString(HUDState.PlayerMovementInputDebug.IsEmpty() ? TEXT("None") : HUDState.PlayerMovementInputDebug),
			FText::FromString(HUDState.bFirstPersonCamera ? TEXT("FirstPerson") : TEXT("ThirdPerson")),
			FText::FromString(HUDState.InteractionDebugText.IsEmpty() ? TEXT("None") : HUDState.InteractionDebugText)));
	}
	if (AIText)
	{
		const FString AIState = HUDState.NearestAIStateTag.IsValid() ? HUDState.NearestAIStateTag.ToString() : TEXT("None");
		const FString CombatDebug = HUDState.LastCombatDebugText.IsEmpty() ? TEXT("None") : HUDState.LastCombatDebugText;
		const FString VehicleState = HUDState.NearestVehicleStateTag.IsValid() ? HUDState.NearestVehicleStateTag.ToString() : TEXT("None");
		AIText->SetText(FText::Format(
			NSLOCTEXT("FW", "HUDAIDebug", "AI: {0}/{1} alive\nNearest AI: {2}/{3} | {4}\nVehicles: {5}/{6} active\nNearest Vehicle: {7}/{8} | {9}\nCombat: {10}"),
			FText::AsNumber(HUDState.AliveAICount),
			FText::AsNumber(HUDState.AICount),
			FText::AsNumber(FMath::RoundToInt(HUDState.NearestAIHealth)),
			FText::AsNumber(FMath::RoundToInt(HUDState.NearestAIMaxHealth)),
			FText::FromString(AIState),
			FText::AsNumber(HUDState.ActiveVehicleCount),
			FText::AsNumber(HUDState.VehicleCount),
			FText::AsNumber(FMath::RoundToInt(HUDState.NearestVehicleHealth)),
			FText::AsNumber(FMath::RoundToInt(HUDState.NearestVehicleMaxHealth)),
			FText::FromString(VehicleState),
			FText::FromString(CombatDebug)));
	}
}

FText UFWHUDWidget::FormatHealthLine(const TCHAR* Label, float CurrentValue, float MaxValue)
{
	const int32 RoundedCurrent = FMath::RoundToInt(CurrentValue);
	const int32 RoundedMax = FMath::RoundToInt(MaxValue);
	return FText::Format(
		NSLOCTEXT("FW", "HUDHealthLine", "{0}: {1}/{2}"),
		FText::FromString(Label),
		FText::AsNumber(RoundedCurrent),
		FText::AsNumber(RoundedMax));
}
