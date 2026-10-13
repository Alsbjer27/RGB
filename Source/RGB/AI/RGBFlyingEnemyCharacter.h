// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RGBEnemyCharacter.h"
#include "RGBFlyingEnemyCharacter.generated.h"

class ACharacter;
class UStaticMeshComponent;

/**
 * Spherical enemy that pursues the player freely in the side-view X/Z plane.
 * Character movement supplies swept collision while local steering keeps the
 * enemy clear of level geometry and other enemies.
 */
UCLASS()
class RGB_API ARGBFlyingEnemyCharacter : public ARGBEnemyCharacter
{
	GENERATED_BODY()

public:
	ARGBFlyingEnemyCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual ERGBEnemyMobilityType GetEnemyMobilityType() const override { return ERGBEnemyMobilityType::Flying; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void ApplyColorMaterial(ERGBColor Color) override;
	virtual bool IsPlayerWithinAttackRange(const ACharacter* Player) const override;
	virtual void HandleArenaAIActiveChanged(bool bNewActive) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Flying|Visuals")
	TObjectPtr<UStaticMeshComponent> FlyingBody;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Flying|Movement", meta = (ClampMin = "0.0", Units = "cm/s"))
	float MaximumFlightSpeed = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Flying|Movement", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float FlightAcceleration = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Flying|Movement", meta = (ClampMin = "0.0", Units = "cm/s^2"))
	float FlightDeceleration = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Flying|Tracking", meta = (ClampMin = "0.0", Units = "cm"))
	float PlayerArrivalRadius = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Flying|Tracking", meta = (ClampMin = "0.0"))
	float PlayerTrackingWeight = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Flying|Tracking", meta = (ClampMin = "0.01", Units = "s"))
	float TargetRefreshInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Flying|Avoidance", meta = (ClampMin = "0.0", Units = "cm"))
	float EnemySeparationRadius = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Flying|Avoidance", meta = (ClampMin = "0.0"))
	float EnemySeparationWeight = 1.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Flying|Avoidance", meta = (ClampMin = "0.0", Units = "cm"))
	float ObstacleProbeDistance = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Flying|Avoidance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ObstacleProbeRadiusScale = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Flying|Avoidance", meta = (ClampMin = "0.0"))
	float ObstacleAvoidanceWeight = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Flying|Visuals")
	bool bOrientVisualToFlightDirection = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Flying|Visuals", meta = (EditCondition = "bOrientVisualToFlightDirection", ClampMin = "0.0", Units = "deg/s"))
	float VisualRotationSpeed = 540.0f;

private:
	void RefreshPlayerTarget();
	void UpdateFlight(float DeltaTime);
	FVector CalculateSeparationSteering() const;
	FVector ChooseClearFlightDirection(const FVector& DesiredDirection) const;
	float MeasureObstacleClearance(const FVector& Direction, FHitResult* OutHit = nullptr) const;
	FVector ConstrainMoveDeltaToArena(const FVector& StartLocation, const FVector& DesiredDelta) const;
	void UpdateVisualFacing(const FVector& FlightDirection, float DeltaTime);

	UPROPERTY(Transient)
	TWeakObjectPtr<ACharacter> FlightTargetPlayer;

	FTimerHandle TargetRefreshTimer;
	FVector FlightVelocity = FVector::ZeroVector;
	float FlightPlaneY = 0.0f;
	float HorizontalBlockDuration = 0.0f;
	float NextHorizontalBlockLogTime = 0.0f;
	bool bReportedMissingPlayer = false;
};
