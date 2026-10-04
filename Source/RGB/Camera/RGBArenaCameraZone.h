// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "RGBArenaCameraZone.generated.h"

class APawn;
class APlayerController;
class UBoxComponent;
class UCameraComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class ERGBArenaViewMode : uint8
{
	UseManagerDefault,
	NormalPerspective,
	FlatPerspective
};

UENUM(BlueprintType)
enum class ERGBArenaCameraTrackingMode : uint8 {
	Fixed,
	FollowPlayerZ,
	FollowPlayerXZ
};

UCLASS()
class RGB_API ARGBArenaCameraZone : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARGBArenaCameraZone();

	virtual void Tick(float DeltaTime) override;

	ERGBArenaViewMode GetArenaViewMode() const { return ArenaViewMode; }

	bool ContainWorldLocation(const FVector& WorldLocation) const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Camera")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Camera")
	TObjectPtr<UBoxComponent> ArenaBounds;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Camera")
	TObjectPtr<UCameraComponent> ArenaCamera;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Camera", meta = (ClampMin = "0.0", Units = "s"))
	float BlendDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Camera")
	ERGBArenaViewMode ArenaViewMode = ERGBArenaViewMode::UseManagerDefault;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Camera")
	ERGBArenaCameraTrackingMode CameraTrackingMode = ERGBArenaCameraTrackingMode::Fixed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Camera")
	float VerticalFollowOffset = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Camera", meta = (ClampMin = "0.0"))
	float VerticalFollowSpeed = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Camera", meta = (EditCondition = "CameraTrackingMode == ERGBArenaCameraTrackingMode::FollowPlayerXZ"))
	float HorizontalFollowOffset = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Camera", meta = (ClampMin = "0.0", EditCondition = "CameraTrackingMode == ERGBArenaCameraTrackingMode::FollowPlayerXZ"))
	float HorizontalFollowSpeed = 5.0f;

private:
	void UpdateCameraZone();

	FTimerHandle CameraZoneTimer;

	TWeakObjectPtr<APlayerController> TrackedController;
	TWeakObjectPtr<APawn> TrackedPawn;

	bool bArenaViewActive = false;

	void UpdatePlayerFollow(float DeltaTime, bool bSnapImmediately);
};
