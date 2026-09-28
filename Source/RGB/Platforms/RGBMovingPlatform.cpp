// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBMovingPlatform.h"

#include "RGBPathMovementComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
ARGBMovingPlatform::ARGBMovingPlatform()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	SceneRoot->SetMobility(EComponentMobility::Movable);

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));

	PlatformMesh->SetupAttachment(SceneRoot);
	PlatformMesh->SetMobility(EComponentMobility::Movable);
	PlatformMesh->SetCollisionProfileName(TEXT("BlockAll"));
	PlatformMesh->SetSimulatePhysics(false);

	PathMovement = CreateDefaultSubobject<URGBPathMovementComponent>(TEXT("PathMovement"));
}
