// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGBMovingPlatform.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class URGBPathMovementComponent;

UCLASS()
class RGB_API ARGBMovingPlatform : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARGBMovingPlatform();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Moving Platform")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Moving Platform")
	TObjectPtr<UStaticMeshComponent> PlatformMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Moving Platform")
	TObjectPtr<URGBPathMovementComponent> PathMovement;

};
