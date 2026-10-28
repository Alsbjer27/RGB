// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RGBMainMenuPlayerController.generated.h"

/**
 * 
 */

class UInputAction;
class UInputMappingContext;
class ARGBMainMenuDirector;
struct FInputActionValue;

UENUM(BlueprintType)
enum class ERGBMainMenuOption : uint8
{
	Start,
	Settings,
	Credits,
	Exit
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRGBMainMenuSelectionChangedSignature, ERGBMainMenuOption, NewSelection);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRGBMenuLevelTransitionStartedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRGBMenuCreditsVisibilitySignature, bool, bVisible);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRGBMenuSettingsVisibilitySignature, bool, bVisible);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRGBMenuSettingsRowSignature, int32, NewRow);

UCLASS()
class RGB_API ARGBMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARGBMainMenuPlayerController();

	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintPure, Category = "RGB|Main Menu")
	ERGBMainMenuOption GetSelectedOption() const {
		return SelectedOption;
	}
	
	UPROPERTY(BlueprintAssignable, Category = "RGB|Main Menu")
	FRGBMainMenuSelectionChangedSignature OnSelectionChanged;

	UPROPERTY(BlueprintAssignable, Category = "RGB|Main Menu")
	FRGBMenuLevelTransitionStartedSignature OnLevelTransitionStarted;

	/** Opens after the credits walk finishes; Back (or Confirm) closes it again. */
	UPROPERTY(BlueprintAssignable, Category = "RGB|Main Menu")
	FRGBMenuCreditsVisibilitySignature OnCreditsVisibilityChanged;

	UFUNCTION(BlueprintPure, Category = "RGB|Main Menu")
	bool IsCreditsOpen() const { return bCreditsOpen; }

	/** Opens after the settings walk finishes; Back closes it again. */
	UPROPERTY(BlueprintAssignable, Category = "RGB|Main Menu")
	FRGBMenuSettingsVisibilitySignature OnSettingsVisibilityChanged;

	UPROPERTY(BlueprintAssignable, Category = "RGB|Main Menu")
	FRGBMenuSettingsRowSignature OnSettingsRowChanged;

	UFUNCTION(BlueprintPure, Category = "RGB|Main Menu")
	bool IsSettingsOpen() const { return bSettingsOpen; }

	/** 0 = Difficulty, 1 = Master Volume. */
	UFUNCTION(BlueprintPure, Category = "RGB|Main Menu")
	int32 GetSelectedSettingsRow() const { return SettingsRow; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Main Menu|Input")
	TObjectPtr<UInputMappingContext> MenuMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Main Menu|Input")
	TObjectPtr<UInputAction> NavigateAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Main Menu|Input")
	TObjectPtr<UInputAction> ConfirmAction;

	/** Closes an open menu page (Credits, later Settings). Map e.g. Escape and Gamepad Face Button Right. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Main Menu|Input")
	TObjectPtr<UInputAction> BackAction;

	/** Left/right on the Settings page (1D axis: right/positive increases). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Main Menu|Input")
	TObjectPtr<UInputAction> AdjustAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Main Menu|Input", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NavigationThreshold = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Main Menu|Levels")
	FName GameplayLevelName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Main Menu|Transition", meta = (ClampMin = "0.0", Units = "s"))
	float LevelFadeOutDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Main Menu|Transition", meta = (ClampMin = "0.0", Units = "s"))
	float LevelBlackDelay = 0.25f;

private:
	void HandleNavigate(const FInputActionValue& Value);
	void HandleNavigateReleased(const FInputActionValue& Value);
	void HandleConfirm(const FInputActionValue& Value);
	void HandleBack(const FInputActionValue& Value);
	void HandleAdjust(const FInputActionValue& Value);
	void HandleAdjustReleased(const FInputActionValue& Value);
	void SetSelectedOption(ERGBMainMenuOption NewSelection);

	UFUNCTION()
	void HandleStartReady();

	UFUNCTION()
	void HandleCreditsReady();

	void SetCreditsOpen(bool bOpen);
	bool bCreditsOpen = false;

	UFUNCTION()
	void HandleSettingsReady();

	void SetSettingsOpen(bool bOpen);
	void AdjustSelectedSetting(int32 Step);
	bool bSettingsOpen = false;
	int32 SettingsRow = 0;
	bool bAdjustHeld = false;

	void OpenGameplayLevel();
	FTimerHandle LevelTransitionTimerHandle;
	bool bLevelTransitionActive = false;

	TWeakObjectPtr<ARGBMainMenuDirector> MenuDirector;

	UPROPERTY(VisibleInstanceOnly, Category = "RGB|Main Menu")
	ERGBMainMenuOption SelectedOption = ERGBMainMenuOption::Start;

	bool bNavigationHeld = false;
};
