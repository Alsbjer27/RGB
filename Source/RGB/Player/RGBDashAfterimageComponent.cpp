// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBDashAfterimageComponent.h"

#include "RGBDashAfterImage.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"

// Sets default values for this component's properties
URGBDashAfterimageComponent::URGBDashAfterimageComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}

void URGBDashAfterimageComponent::StartTrail()
{
	if (!IsValid(AfterimageMaterial)) {
		UE_LOG(LogTemp, Warning, TEXT("%s cannot start its dash afterimage trail: " "AfterimageMaterial is not assigned."), *GetNameSafe(GetOwner()));
		return;
	}

	UWorld* World = GetWorld();

	if (!IsValid(World)) {
		return;
	}

	StopTrail();

	bTrailActive = true;

	SpawnAfterimage();

	const float SafeSpawnInterval = FMath::Max(SpawnInterval, 0.01f);
	const float SafeTrailDuration = FMath::Max(TrailDuration, 0.01f);

	World->GetTimerManager().SetTimer(SpawnTimerHandle, this, &URGBDashAfterimageComponent::SpawnAfterimage, SafeSpawnInterval, true);
	World->GetTimerManager().SetTimer(StopTimerHandle, this, &URGBDashAfterimageComponent::StopTrail, SafeTrailDuration, false);
}

void URGBDashAfterimageComponent::StopTrail()
{
	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
		World->GetTimerManager().ClearTimer(StopTimerHandle);
	}

	bTrailActive = false;
}

void URGBDashAfterimageComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopTrail();
	Super::EndPlay(EndPlayReason);
}

void URGBDashAfterimageComponent::SpawnAfterimage()
{
	if (!bTrailActive || !IsValid(AfterimageMaterial)) {
		return;
	}

	ACharacter* Character = Cast<ACharacter>(GetOwner());

	if (!IsValid(Character) || !IsValid(Character->GetMesh())) {
		StopTrail();
		return;
	}

	UWorld* World = GetWorld();

	if (!IsValid(World)) {
		StopTrail();
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ARGBDashAfterImage* Afterimage = World->SpawnActor<ARGBDashAfterImage>(ARGBDashAfterImage::StaticClass(), FTransform::Identity, SpawnParameters);

	if (IsValid(Afterimage)) {
		Afterimage->InitializeAfterImage(Character->GetMesh(), AfterimageMaterial, AfterimageLifetime, AfterimageColor, InitialOpacity);
	}
}

