// Self-contained main-menu Settings page: background, rows, highlight and visibility.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RGBSettingsPageWidget.generated.h"

class UTextBlock;
class UImage;
class UTexture2D;
class ARGBMainMenuPlayerController;
class URGBGameSettingsSubsystem;

/**
 * Place one instance inside the main menu widget (full-screen anchors). It builds its own content,
 * hides itself until the menu controller opens the Settings page, and reflects the controller's
 * selected row and the session settings. No Blueprint graph is required.
 */
UCLASS(meta = (DisplayName = "RGB Settings Page"))
class RGB_API URGBSettingsPageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URGBSettingsPageWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Settings Page")
	TSoftObjectPtr<UTexture2D> BackgroundTexture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Settings Page", meta = (ClampMin = "8"))
	int32 FontSize = 40;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Settings Page")
	FLinearColor SelectedColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Settings Page")
	FLinearColor UnselectedColor = FLinearColor(0.32f, 0.32f, 0.32f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Settings Page")
	FLinearColor ValueHighlightColor = FLinearColor(0.27f, 0.92f, 0.37f, 1.0f);

private:
	void BuildContent();
	void Refresh();

	UFUNCTION()
	void HandleVisibilityChanged(bool bVisible);

	UFUNCTION()
	void HandleRowChanged(int32 NewRow);

	UFUNCTION()
	void HandleSettingsChanged();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> LabelTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> ValueTexts;

	TWeakObjectPtr<ARGBMainMenuPlayerController> BoundController;
	TWeakObjectPtr<URGBGameSettingsSubsystem> BoundSettings;
};
