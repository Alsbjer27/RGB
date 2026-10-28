// Self-contained main-menu Settings page: background, rows, highlight and visibility.

#include "RGBSettingsPageWidget.h"
#include "RGBMainMenuPlayerController.h"
#include "../Game/RGBGameSettingsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"

namespace
{
	const TCHAR* const RowLabels[] = { TEXT("DIFFICULTY"), TEXT("MASTER VOLUME") };
	constexpr int32 RowCount = UE_ARRAY_COUNT(RowLabels);
}

URGBSettingsPageWidget::URGBSettingsPageWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BackgroundTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/RGB/UI/MainMenu/T_MenuSettings.T_MenuSettings")));
}

TSharedRef<SWidget> URGBSettingsPageWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) {
		BuildContent();
	}
	return Super::RebuildWidget();
}

void URGBSettingsPageWidget::BuildContent()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SettingsRoot"));
	WidgetTree->RootWidget = Root;

	UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SettingsBackground"));
	if (UTexture2D* Texture = BackgroundTexture.LoadSynchronous()) {
		Background->SetBrushFromTexture(Texture, false);
	}
	else {
		Background->SetColorAndOpacity(FLinearColor(0.02f, 0.03f, 0.05f, 0.92f));
	}
	if (UOverlaySlot* BackgroundSlot = Root->AddChildToOverlay(Background)) {
		BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
		BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* Rows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsRows"));
	if (UOverlaySlot* RowsSlot = Root->AddChildToOverlay(Rows)) {
		RowsSlot->SetHorizontalAlignment(HAlign_Center);
		RowsSlot->SetVerticalAlignment(VAlign_Center);
	}

	LabelTexts.Reset();
	ValueTexts.Reset();
	for (int32 Index = 0; Index < RowCount; ++Index) {
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (UVerticalBoxSlot* RowSlot = Rows->AddChildToVerticalBox(Row)) {
			RowSlot->SetPadding(FMargin(0.0f, 18.0f));
		}
		for (int32 Column = 0; Column < 2; ++Column) {
			USizeBox* Cell = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			Cell->SetWidthOverride(Column == 0 ? 520.0f : 420.0f);
			UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			FSlateFontInfo Font = Text->GetFont();
			Font.Size = FontSize;
			Font.TypefaceFontName = TEXT("Bold");
			Text->SetFont(Font);
			Text->SetJustification(Column == 0 ? ETextJustify::Left : ETextJustify::Center);
			Cell->AddChild(Text);
			if (UHorizontalBoxSlot* CellSlot = Row->AddChildToHorizontalBox(Cell)) {
				CellSlot->SetVerticalAlignment(VAlign_Center);
			}
			(Column == 0 ? LabelTexts : ValueTexts).Add(Text);
		}
	}
	Refresh();
}

void URGBSettingsPageWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (IsDesignTime()) {
		return;
	}

	if (ARGBMainMenuPlayerController* Controller = Cast<ARGBMainMenuPlayerController>(GetOwningPlayer())) {
		Controller->OnSettingsVisibilityChanged.AddUniqueDynamic(this, &URGBSettingsPageWidget::HandleVisibilityChanged);
		Controller->OnSettingsRowChanged.AddUniqueDynamic(this, &URGBSettingsPageWidget::HandleRowChanged);
		BoundController = Controller;
	}
	if (const UGameInstance* GameInstance = GetGameInstance()) {
		if (URGBGameSettingsSubsystem* Settings = GameInstance->GetSubsystem<URGBGameSettingsSubsystem>()) {
			Settings->OnSettingsChanged.AddUniqueDynamic(this, &URGBSettingsPageWidget::HandleSettingsChanged);
			BoundSettings = Settings;
		}
	}
	HandleVisibilityChanged(BoundController.IsValid() && BoundController->IsSettingsOpen());
	Refresh();
}

void URGBSettingsPageWidget::NativeDestruct()
{
	if (ARGBMainMenuPlayerController* Controller = BoundController.Get()) {
		Controller->OnSettingsVisibilityChanged.RemoveDynamic(this, &URGBSettingsPageWidget::HandleVisibilityChanged);
		Controller->OnSettingsRowChanged.RemoveDynamic(this, &URGBSettingsPageWidget::HandleRowChanged);
	}
	if (URGBGameSettingsSubsystem* Settings = BoundSettings.Get()) {
		Settings->OnSettingsChanged.RemoveDynamic(this, &URGBSettingsPageWidget::HandleSettingsChanged);
	}
	BoundController.Reset();
	BoundSettings.Reset();
	Super::NativeDestruct();
}

void URGBSettingsPageWidget::HandleVisibilityChanged(bool bVisible)
{
	SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	if (bVisible) {
		Refresh();
	}
}

void URGBSettingsPageWidget::HandleRowChanged(int32 NewRow)
{
	Refresh();
}

void URGBSettingsPageWidget::HandleSettingsChanged()
{
	Refresh();
}

void URGBSettingsPageWidget::Refresh()
{
	const URGBGameSettingsSubsystem* Settings = BoundSettings.Get();
	const int32 SelectedRow = BoundController.IsValid() ? BoundController->GetSelectedSettingsRow() : 0;

	for (int32 Index = 0; Index < RowCount && Index < LabelTexts.Num() && Index < ValueTexts.Num(); ++Index) {
		FString Value;
		if (Index == 0) {
			const ERGBDifficulty Difficulty = Settings ? Settings->GetDifficulty() : ERGBDifficulty::Medium;
			Value = Difficulty == ERGBDifficulty::Easy ? TEXT("EASY") : Difficulty == ERGBDifficulty::Hard ? TEXT("HARD") : TEXT("MEDIUM");
		}
		else {
			Value = FString::Printf(TEXT("%d %%"), FMath::RoundToInt((Settings ? Settings->GetMasterVolume() : 1.0f) * 100.0f));
		}

		const bool bSelected = Index == SelectedRow;
		LabelTexts[Index]->SetText(FText::FromString(RowLabels[Index]));
		LabelTexts[Index]->SetColorAndOpacity(FSlateColor(bSelected ? SelectedColor : UnselectedColor));
		ValueTexts[Index]->SetText(FText::FromString(bSelected ? FString::Printf(TEXT("<   %s   >"), *Value) : Value));
		ValueTexts[Index]->SetColorAndOpacity(FSlateColor(bSelected ? ValueHighlightColor : UnselectedColor));
	}
}
