// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RGBPlayerController.generated.h"


/**
 * 
 */

class UInputMappingContext;
class UInputAction;

UCLASS()
class RGB_API ARGBPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARGBPlayerController();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Transition", meta = (ClampMin = "0.0", Units = "s"))
	float InitialFadeInDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Transition", meta = (ClampMin = "0.0", Units = "s"))
	float InitialBlackDelay = 1.5f;

	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Input")
	TObjectPtr<UInputAction> RestartAction;

	void RestartPlayer();

	void StartInitialFadeIn();
	void FinishInitialFadeIn();

	FTimerHandle InitialBlackDelayTimerHandle;
	FTimerHandle InitialFadeInTimerHandle;
};
