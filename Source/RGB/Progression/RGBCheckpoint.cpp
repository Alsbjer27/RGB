// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBCheckpoint.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"

#include "../Game/RGBGameMode.h"
#include "../Player/RGBPlayerCharacter.h"
#include "Engine/World.h"

// Sets default values
ARGBCheckpoint::ARGBCheckpoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ActivationTrigger = CreateDefaultSubobject<UBoxComponent>("ActivationTrigger");
	ActivationTrigger->SetupAttachment(SceneRoot);
	ActivationTrigger->SetBoxExtent(FVector(100.f, 250.0f, 150.0f));
	ActivationTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ActivationTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	ActivationTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ActivationTrigger->SetGenerateOverlapEvents(true);
	ActivationTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ARGBCheckpoint::HandleActivationOverlap);

	RespawnTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("RespawnTrigger"));
	RespawnTrigger->SetupAttachment(SceneRoot);
	RespawnTrigger->SetBoxExtent(FVector(500.0f, 500.0f, 50.0f));
	RespawnTrigger->SetRelativeLocation(FVector(0.0f, 0.0f, -500.0f));
	RespawnTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	RespawnTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	RespawnTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RespawnTrigger->SetGenerateOverlapEvents(true);
	RespawnTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ARGBCheckpoint::HandleRespawnOverlap);

	RespawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RespawnPoint"));
	RespawnPoint->SetupAttachment(SceneRoot);
	RespawnPoint->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	
	RespawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("RespawnDirection"));
	RespawnDirection->SetupAttachment(RespawnPoint);
	RespawnDirection->SetArrowColor(FColor::Green);
}

FTransform ARGBCheckpoint::GetRespawnTransform() const
{
	return IsValid(RespawnPoint) ? RespawnPoint->GetComponentTransform() : GetActorTransform();
}

void ARGBCheckpoint::HandleActivationOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!IsValid(Cast<ARGBPlayerCharacter>(OtherActor))) {
		return;
	}

	ARGBGameMode* GameMode = GetWorld()->GetAuthGameMode<ARGBGameMode>();
	
	if (IsValid(GameMode)) {
		GameMode->SetActiveCheckpoint(this);
	}
}

void ARGBCheckpoint::HandleRespawnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ARGBPlayerCharacter* Player = Cast<ARGBPlayerCharacter>(OtherActor);
	
	if (!IsValid(Player)) {
		return;
	}

	ARGBGameMode* GameMode = GetWorld()->GetAuthGameMode<ARGBGameMode>();
	if (!IsValid(GameMode)) {
		return;
	}

	GameMode->SetActiveCheckpoint(this);

	AController* PlayerController = Player->GetController();
	if (IsValid(PlayerController)) {
		GameMode->StartRespawnTransition(PlayerController);
	}
}