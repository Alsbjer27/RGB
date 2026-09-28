// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGBColorComponent.h"
#include <Engine/EngineTypes.h>
#include "RGBColorGroupActivator.generated.h"

class ARGBColorPlatform;

UCLASS()
class RGB_API ARGBColorGroupActivator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARGBColorGroupActivator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Color Activation")
	TArray<TObjectPtr<ARGBColorPlatform>> WatchedPlatforms;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Color Activation")
	TArray<TObjectPtr<AActor>> ControlledActors;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Color Activation")
	bool bRequireSpecificColor = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Color Activation", meta = (EditCondition = "bRequireSpecificColor"))
	ERGBColor RequiredColor = ERGBColor::Blue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Color Activation")
	bool bDisableWhenColorDiffer = true;

private:
	UFUNCTION()
	void HandlePlatformColorChanged(ERGBColor PreviousColor, ERGBColor NewColor);

	void EvaluateColorCondition();
	void SetControlledMovementEnabled(bool bEnabled);
};
