// Full-screen story card: title and narrative fade in on black, then a continue prompt appears.

#include "RGBLevelIntroWidget.h"
#include "../Progression/RGBLevelIntro.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	UTextBlock* MakeText(UWidgetTree* Tree, int32 Size, const TCHAR* Typeface, const FLinearColor& Color)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Font.TypefaceFontName = Typeface;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetJustification(ETextJustify::Center);
		Text->SetAutoWrapText(true);
		return Text;
	}
}

TSharedRef<SWidget> URGBLevelIntroWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget) {
		BuildContent();
	}
	return Super::RebuildWidget();
}

void URGBLevelIntroWidget::BuildContent()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("IntroRoot"));
	WidgetTree->RootWidget = Root;

	// Solid black behind the text so the HUD and the world stay hidden.
	UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("IntroBackground"));
	Background->SetBrushColor(FLinearColor::Black);
	if (UOverlaySlot* BackgroundSlot = Root->AddChildToOverlay(Background)) {
		BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
		BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
	}

	USizeBox* Column = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	Column->SetMaxDesiredWidth(1250.0f);
	if (UOverlaySlot* ColumnSlot = Root->AddChildToOverlay(Column)) {
		ColumnSlot->SetHorizontalAlignment(HAlign_Center);
		ColumnSlot->SetVerticalAlignment(VAlign_Center);
	}

	TextColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChild(TextColumn);

	TitleText = MakeText(WidgetTree, 54, TEXT("Bold"), FLinearColor::White);
	BodyText = MakeText(WidgetTree, 28, TEXT("Regular"), FLinearColor(0.75f, 0.78f, 0.85f, 1.0f));
	PromptTextBlock = MakeText(WidgetTree, 24, TEXT("Bold"), FLinearColor(1.0f, 0.62f, 0.02f, 1.0f));

	if (UVerticalBoxSlot* TitleSlot = TextColumn->AddChildToVerticalBox(TitleText)) {
		TitleSlot->SetHorizontalAlignment(HAlign_Fill);
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 36.0f));
	}
	if (UVerticalBoxSlot* BodySlot = TextColumn->AddChildToVerticalBox(BodyText)) {
		BodySlot->SetHorizontalAlignment(HAlign_Fill);
		BodySlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 70.0f));
	}
	if (UVerticalBoxSlot* PromptSlot = TextColumn->AddChildToVerticalBox(PromptTextBlock)) {
		PromptSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	TitleText->SetText(PendingTitle);
	BodyText->SetText(PendingBody);
	PromptTextBlock->SetText(PendingPrompt);
	PromptTextBlock->SetRenderOpacity(0.0f);
	ApplyTextOpacity(0.0f);
}

void URGBLevelIntroWidget::Setup(const ARGBLevelIntro* Intro)
{
	if (IsValid(Intro)) {
		SetupText(Intro->Title, Intro->Body, Intro->PromptText, Intro->FadeInDuration, Intro->PromptDelay, Intro->FadeOutDuration);
	}
}

void URGBLevelIntroWidget::SetupText(const FText& Title, const FText& Body, const FText& Prompt, float InFadeInDuration, float InPromptDelay, float InFadeOutDuration)
{
	PendingTitle = Title;
	PendingBody = Body;
	PendingPrompt = Prompt;
	FadeInDuration = FMath::Max(InFadeInDuration, 0.0f);
	PromptDelay = FMath::Max(InPromptDelay, 0.0f);
	FadeOutDuration = FMath::Max(InFadeOutDuration, 0.0f);
	Stage = EStage::FadingIn;
	StageTime = 0.0f;
	TotalTime = 0.0f;

	if (TitleText) {
		TitleText->SetText(PendingTitle);
		BodyText->SetText(PendingBody);
		PromptTextBlock->SetText(PendingPrompt);
		PromptTextBlock->SetRenderOpacity(0.0f);
		ApplyTextOpacity(0.0f);
	}
}

void URGBLevelIntroWidget::Continue()
{
	switch (Stage) {
	case EStage::FadingIn:
	case EStage::Waiting:
		// First press: skip to the finished card with the prompt showing.
		Stage = EStage::PromptVisible;
		StageTime = 0.0f;
		ApplyTextOpacity(1.0f);
		if (PromptTextBlock) {
			PromptTextBlock->SetRenderOpacity(1.0f);
		}
		break;
	case EStage::PromptVisible:
		Stage = EStage::FadingOut;
		StageTime = 0.0f;
		break;
	default:
		break;
	}
}

void URGBLevelIntroWidget::ApplyTextOpacity(float Opacity)
{
	if (TitleText) {
		TitleText->SetRenderOpacity(Opacity);
	}
	if (BodyText) {
		BodyText->SetRenderOpacity(Opacity);
	}
}

void URGBLevelIntroWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Clamp a long first frame after level load so the fade is still visible.
	const float Delta = FMath::Min(InDeltaTime, 0.1f);
	StageTime += Delta;
	TotalTime += Delta;

	switch (Stage) {
	case EStage::FadingIn:
		ApplyTextOpacity(FadeInDuration > 0.0f ? FMath::Clamp(StageTime / FadeInDuration, 0.0f, 1.0f) : 1.0f);
		if (StageTime >= FadeInDuration) {
			Stage = EStage::Waiting;
		}
		break;
	case EStage::Waiting:
		if (TotalTime >= PromptDelay) {
			Stage = EStage::PromptVisible;
			StageTime = 0.0f;
		}
		break;
	case EStage::PromptVisible:
		if (PromptTextBlock) {
			PromptTextBlock->SetRenderOpacity(FMath::Clamp(StageTime / 0.4f, 0.0f, 1.0f));
		}
		break;
	case EStage::FadingOut:
	{
		const float Alpha = FadeOutDuration > 0.0f ? 1.0f - FMath::Clamp(StageTime / FadeOutDuration, 0.0f, 1.0f) : 0.0f;
		ApplyTextOpacity(Alpha);
		if (PromptTextBlock) {
			PromptTextBlock->SetRenderOpacity(Alpha);
		}
		if (StageTime >= FadeOutDuration) {
			Stage = EStage::Done;
			OnFinished.Broadcast();
		}
		break;
	}
	default:
		break;
	}
}
