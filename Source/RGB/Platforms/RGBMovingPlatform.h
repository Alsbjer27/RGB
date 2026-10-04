// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGBRotationComponent.h"
#include "RGBMovingPlatform.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class URGBPathMovementComponent;
class ARGBPlatformPath;
class URGBRotationComponent;

UCLASS()
class RGB_API ARGBMovingPlatform : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARGBMovingPlatform();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Moving Platform")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Moving Platform")
	TObjectPtr<UStaticMeshComponent> PlatformMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Moving Platform")
	TObjectPtr<URGBPathMovementComponent> PathMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Moving Platform")
	TObjectPtr<URGBRotationComponent> RotationMovement;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Moving Platform")
	TObjectPtr<ARGBPlatformPath> MovementPath;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Moving Platform", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StartingPathProgress = 0.0f;
};
