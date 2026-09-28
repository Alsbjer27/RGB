// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBPathMovementComponent.h"

#include "RGBPlatformPath.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Actor.h"


// Sets default values for this component's properties
URGBPathMovementComponent::URGBPathMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}


// Called when the game starts
void URGBPathMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();

	if (!IsValid(Owner) || !IsValid(MovementPath) || !IsValid(MovementPath->GetPathSpline())) {
		UE_LOG(LogTemp, Warning, TEXT("%s: Path Movement Component has no valid Movement Path."), *GetNameSafe(Owner));
		SetComponentTickEnabled(false);
		return;
	}

	if (USceneComponent* Root = Owner->GetRootComponent()) {
		Root->SetMobility(EComponentMobility::Movable);
	}

	DistanceAlongPath = 0.0f;
	MovementDirection = 1.0f;
	
	if (bSnapToPathStart) {
		ApplyPathTransform();
	}

	SetMovementEnabled(bStartActive);
}

void URGBPathMovementComponent::ApplyPathTransform()
{
	AActor* Owner = GetOwner();

	if (!IsValid(Owner) || !IsValid(MovementPath)) {
		return;
	}

	USplineComponent* Spline = MovementPath->GetPathSpline();

	if (!IsValid(Spline)) {
		return;
	}

	const FVector TargetLocation = Spline->GetLocationAtDistanceAlongSpline(DistanceAlongPath, ESplineCoordinateSpace::World);
	FRotator TargetRotation = Owner->GetActorRotation();

	if (bFollowPathRotation) {
		TargetRotation = Spline->GetRotationAtDistanceAlongSpline(DistanceAlongPath, ESplineCoordinateSpace::World);
	}

	Owner->SetActorLocationAndRotation(TargetLocation, TargetRotation, false, nullptr, ETeleportType::None);
}


// Called every frame
void URGBPathMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bMovementEnabled || !IsValid(MovementPath)) {
		return;
	}
	
	USplineComponent* Spline = MovementPath->GetPathSpline();

	if (!IsValid(Spline)) {
		SetMovementEnabled(false);
		return;
	}

	const float PathLength = Spline->GetSplineLength();

	if (PathLength <= KINDA_SMALL_NUMBER || MovementSpeed <= 0.0f) {
		return;
	}

	DistanceAlongPath += MovementSpeed * MovementDirection * DeltaTime;

	while (DistanceAlongPath > PathLength || DistanceAlongPath < 0.0f) {
		if (DistanceAlongPath > PathLength) {
			DistanceAlongPath = PathLength - (DistanceAlongPath - PathLength);
			MovementDirection = -1.0f;
		}
		else {
			DistanceAlongPath = -DistanceAlongPath;
			MovementDirection = 1.0f;
		}
	}

	ApplyPathTransform();
}

void URGBPathMovementComponent::SetMovementEnabled(bool bEnabled)
{
	bMovementEnabled = bEnabled && IsValid(MovementPath) && IsValid(MovementPath->GetPathSpline());
	SetComponentTickEnabled(bMovementEnabled);
}

bool URGBPathMovementComponent::IsMovementEnabled() const
{
	return bMovementEnabled;
}

