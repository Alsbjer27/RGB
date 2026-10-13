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

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Main Menu|Input")
	TObjectPtr<UInputMappingContext> MenuMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Main Menu|Input")
	TObjectPtr<UInputAction> NavigateAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Main Menu|Input")
	TObjectPtr<UInputAction> ConfirmAction;

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
	void SetSelectedOption(ERGBMainMenuOption NewSelection);

	UFUNCTION()
	void HandleStartReady();

	void OpenGameplayLevel();
	FTimerHandle LevelTransitionTimerHandle;
	bool bLevelTransitionActive = false;

	TWeakObjectPtr<ARGBMainMenuDirector> MenuDirector;

	UPROPERTY(VisibleInstanceOnly, Category = "RGB|Main Menu")
	ERGBMainMenuOption SelectedOption = ERGBMainMenuOption::Start;

	bool bNavigationHeld = false;
};
