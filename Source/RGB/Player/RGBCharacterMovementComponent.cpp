// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBCharacterMovementComponent.h"

#include "Templates/UnrealTemplate.h"
#include "GameFramework/Character.h"
#include "Components/SceneComponent.h"

float URGBCharacterMovementComponent::GetGravityZ() const
{
	const float BaseGravity = Super::GetGravityZ();

	if (IsFalling() && Velocity.Z <= 0.0f) {
		return BaseGravity * FallingGravityMultiplier;
	}

	return BaseGravity;
}

bool URGBCharacterMovementComponent::TryStartAirDash(float Direction)
{
	if (!HasValidData() || !IsFalling() || bHasAirDashed) {
		return false;
	}

	if (FMath::IsNearlyZero(Direction) || AirDashSpeed <= 0.0f || AirDashDuration <= 0.0f) {
		return false;
	}

	bHasAirDashed = true;
	AirDashDirection = Direction > 0.0f ? 1.0f : -1.0f;
	AirDashTimeRemaining = AirDashDuration;

	CharacterOwner->StopJumping();

	Velocity = FVector(AirDashDirection * AirDashSpeed, 0.0f, 0.0f);

	return true;
}

void URGBCharacterMovementComponent::PhysFalling(float DeltaTime, int32 Iterations)
{
	if (AirDashTimeRemaining > 0.0f) {
		PhysAirDash(DeltaTime, Iterations);
		return;
	}

	if (bHasAirDashed) {
		Super::PhysFalling(DeltaTime, Iterations);
		return;
	}

	const float MaxInputAcceleration = GetMaxAcceleration();

	const float HorizontalInput = MaxInputAcceleration > SMALL_NUMBER ? FMath::Clamp(Acceleration.X / MaxInputAcceleration, -1.0f, 1.0f) : 0.0f;

	Velocity.X = HorizontalInput * GetMaxSpeed();
	Velocity.Y = 0.0f;

	TGuardValue<float> AirControlGuard(AirControl, 0.0f);
	TGuardValue<float> FrictionGuard(FallingLateralFriction, 0.0f);
	TGuardValue<float> BrakingGuard(BrakingDecelerationFalling, 0.0f);

	Super::PhysFalling(DeltaTime, Iterations);
}

void URGBCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (IsMovingOnGround()) {
		bHasAirDashed = false;
		AirDashTimeRemaining = 0.0f;
	}
	else if (!IsFalling()) {
		AirDashTimeRemaining = 0.0f;
	}
}

void URGBCharacterMovementComponent::PhysAirDash(float DeltaTime, int32 Iterations)
{
	if (!HasValidData() || DeltaTime <= 0.0f) {
		return;
	}

	const float DashStep = FMath::Min(DeltaTime, AirDashTimeRemaining);

	Velocity = FVector(AirDashDirection * AirDashSpeed, 0.0f, 0.0f);

	const FVector DashDelta = Velocity * DashStep;

	FHitResult Hit;
	SafeMoveUpdatedComponent(DashDelta, UpdatedComponent->GetComponentQuat(), true, Hit);

	float TimeUsed = DashStep;

	if (Hit.bBlockingHit) {
		TimeUsed = DashStep * Hit.Time;
		AirDashTimeRemaining = 0.0f;
		Velocity = FVector::ZeroVector;

		HandleImpact(Hit, DashStep, DashDelta);
	}
	else {
		AirDashTimeRemaining = FMath::Max(0.0f, AirDashTimeRemaining - DashStep);
	}

	if (!HasValidData()) {
		return;
	}

	const float RemainingTime = DeltaTime - TimeUsed;
	if (AirDashTimeRemaining <= 0.0f && RemainingTime > SMALL_NUMBER) {
		StartNewPhysics(RemainingTime, Iterations + 1);
	}
}
