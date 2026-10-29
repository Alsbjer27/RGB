// Full-screen story card: title and narrative fade in on black, then a continue prompt appears.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RGBLevelIntroWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class ARGBLevelIntro;

DECLARE_MULTICAST_DELEGATE(FRGBLevelIntroFinishedSignature);

/**
 * Built entirely in C++ and animated in real time, so it keeps running while the game is paused.
 * Continue() skips straight to the finished text the first time and fades out the second time.
 */
UCLASS()
class RGB_API URGBLevelIntroWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Setup(const ARGBLevelIntro* Intro);
	void SetupText(const FText& Title, const FText& Body, const FText& Prompt, float InFadeInDuration, float InPromptDelay, float InFadeOutDuration);
	void Continue();

	FRGBLevelIntroFinishedSignature OnFinished;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	enum class EStage : uint8 { FadingIn, Waiting, PromptVisible, FadingOut, Done };

	void BuildContent();
	void ApplyTextOpacity(float Opacity);

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> BodyText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PromptTextBlock;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> TextColumn;

	EStage Stage = EStage::FadingIn;
	float StageTime = 0.0f;
	float TotalTime = 0.0f;
	float FadeInDuration = 1.5f;
	float PromptDelay = 3.5f;
	float FadeOutDuration = 0.8f;
	FText PendingTitle;
	FText PendingBody;
	FText PendingPrompt;
};
