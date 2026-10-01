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
	ArenaControl->OnArenaResetstarted.AddUniqueDynamic(this, &ARGBArenaCompletionActivator::HandleArenaResetStarted);


}

void ARGBArenaCompletionActivator::HandleArenaResetStarted()
{
}
