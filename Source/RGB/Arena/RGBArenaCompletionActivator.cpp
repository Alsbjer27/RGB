// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBArenaCompletionActivator.h"

#include "RGBArenaControl.h"
#include "../Platforms/RGBPathMovementComponent.h"
#include "TimerManager.h"

// Sets default values
ARGBArenaCompletionActivator::ARGBArenaCompletionActivator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void ARGBArenaCompletionActivator::BeginPlay()
{
	Super::BeginPlay();
	
	if (!IsValid(ArenaControl)) {
		UE_LOG(LogTemp, Warning, TEXT("%s has no Arena Control assigned."), *GetName());
		return;
	}

	ArenaControl->OnArenaCompleted.AddUniqueDynamic(this, &ARGBArenaCompletionActivator::HandleArenaCompleted);
	ArenaControl->OnArenaResetStarted.AddUniqueDynamic(this, &ARGBArenaCompletionActivator::HandleArenaResetStarted);

	GetWorldTimerManager().SetTimerForNextTick(this, &ARGBArenaCompletionActivator::InitializeActivationState);
}

void ARGBArenaCompletionActivator::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(ArenaControl)) {
		ArenaControl->OnArenaCompleted.RemoveDynamic(this, &ARGBArenaCompletionActivator::HandleArenaCompleted);
		ArenaControl->OnArenaResetStarted.RemoveDynamic(this, &ARGBArenaCompletionActivator::HandleArenaResetStarted);
	}

	Super::EndPlay(EndPlayReason);
}

void ARGBArenaCompletionActivator::InitializeActivationState()
{
	SetControlledMovementEnabled(IsValid(ArenaControl) && ArenaControl->IsCompleted());
}

void ARGBArenaCompletionActivator::SetControlledMovementEnabled(bool bEnabled)
{
	for (AActor* ControlledActor : ControlledActors) {
		if (!IsValid(ControlledActor)) {
			continue;
		}

		URGBPathMovementComponent* Movement = ControlledActor->FindComponentByClass<URGBPathMovementComponent>();

		if (IsValid(Movement)) {
			Movement->SetMovementEnabled(bEnabled);
		}
		else {
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("%s controls %s, but it has no " "RGB Path Movement Component."), *GetName(), *GetNameSafe(ControlledActor));
		}
	}
}

void ARGBArenaCompletionActivator::ResetControlledMovementToStart()
{
	for (AActor* ControlledActor : ControlledActors) {
		if (!IsValid(ControlledActor)) {
			continue;
		}

		URGBPathMovementComponent* Movement = ControlledActor->FindComponentByClass<URGBPathMovementComponent>();

		if (IsValid(Movement)) {
			Movement->ResetToPathStart();
		}
	}
}

void ARGBArenaCompletionActivator::HandleArenaCompleted()
{
	SetControlledMovementEnabled(true);
}

void ARGBArenaCompletionActivator::HandleArenaResetStarted()
{
	SetControlledMovementEnabled(false);
	ResetControlledMovementToStart();
}
