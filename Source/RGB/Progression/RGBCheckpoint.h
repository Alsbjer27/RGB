// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGBCheckpoint.generated.h"

class UArrowComponent;
class UBoxComponent;
class USceneComponent;

UCLASS()
class RGB_API ARGBCheckpoint : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARGBCheckpoint();
	FTransform GetRespawnTransform() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Checkpoint")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Checkpoint")
	TObjectPtr<UBoxComponent> ActivationTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Checkpoint")
	TObjectPtr<UBoxComponent> RespawnTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Checkpoint")
	TObjectPtr<USceneComponent> RespawnPoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Checkpoint")
	TObjectPtr<UArrowComponent> RespawnDirection;

private:
	UFUNCTION()
	void HandleActivationOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleRespawnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
