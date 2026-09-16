// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBJumpPad.h"

#include "Components/StaticMeshComponent.h"

// Sets default values
ARGBJumpPad::ARGBJumpPad()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	PadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PadMesh"));
	SetRootComponent(PadMesh);

	PadMesh->SetCollisionProfileName(TEXT("BlockAll"));
	PadMesh->SetSimulatePhysics(false);
	PadMesh->SetGenerateOverlapEvents(false);
}


