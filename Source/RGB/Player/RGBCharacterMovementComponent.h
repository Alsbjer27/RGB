// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RGBCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class RGB_API URGBCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
	
public:
	virtual float GetGravityZ() const override;

	bool TryStartAirDash(float Direction);

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Movement", meta = (ClampMin = "1.0"))
	float FallingGravityMultiplier = 2.0f;

	virtual void PhysFalling(float DeltaTime, int32 Iterations) override;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Dash", meta = (ClampMin = "1.0", Units = "cm/s"))
	float AirDashSpeed = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Dash", meta = (ClampMin = "0.01", Units = "s"))
	float AirDashDuration = 0.30f;

private:
	void PhysAirDash(float DeltaTime, int32 Iterations);

	bool bHasAirDashed = false;
	float AirDashTimeRemaining = 0.0f;
	float AirDashDirection = 1.0f;
};
