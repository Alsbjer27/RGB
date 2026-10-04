// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGBArenaCompletionActivator.generated.h"

class ARGBArenaControl;

UCLASS()
class RGB_API ARGBArenaCompletionActivator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARGBArenaCompletionActivator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Arena Activation")
	TObjectPtr<ARGBArenaControl> ArenaControl;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Arena Activation")
	TArray<TObjectPtr<AActor>> ControlledActors;

private:
	UFUNCTION()
	void HandleArenaCompleted();

	UFUNCTION()
	void HandleArenaResetStarted();

	void InitializeActivationState();
	void SetControlledMovementEnabled(bool bEnabled);

	void ResetControlledMovementToStart();
};
