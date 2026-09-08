// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RGBPlayerController.generated.h"

/**
 * 
 */

class UInputMappingContext;

UCLASS()
class RGB_API ARGBPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;
	
};
