// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBAbilityPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

#include "../Player/RGBPlayerCharacter.h"

// Sets default values
ARGBAbilityPickup::ARGBAbilityPickup()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	PickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("PickupCollision"));
	SetRootComponent(PickupCollision);

	PickupCollision->InitSphereRadius(75.0f);
	PickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupCollision->SetGenerateOverlapEvents(true);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(PickupCollision);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// Called when the game starts or when spawned
void ARGBAbilityPickup::BeginPlay()
{
	Super::BeginPlay();

	PickupCollision->OnComponentBeginOverlap.AddDynamic(this, &ARGBAbilityPickup::HandlePickupOverlap);

	const UGameInstance* GameInstance = GetGameInstance();

	if (!IsValid(GameInstance)) {
		return;
	}

	const URGBAbilityUnlockSubsytem* UnlockSubsystem = GameInstance->GetSubsystem<URGBAbilityUnlockSubsytem>();

	if (IsValid(UnlockSubsystem) && UnlockSubsystem->IsAbilityUnlocked(GrantedAbility)) {
		Destroy();
	}
}

void ARGBAbilityPickup::HandlePickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ARGBPlayerCharacter* Player = Cast<ARGBPlayerCharacter>(OtherActor);

	if (!IsValid(Player)) {
		return;
	}

	Player->GrantAbility(GrantedAbility);
	Destroy();
}