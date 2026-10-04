// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBProjectile.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInterface.h"

#include "../AI/RGBEnemyCharacter.h"

#include "Kismet/GameplayStatics.h"

// Sets default values
ARGBProjectile::ARGBProjectile()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	InitialLifeSpan = 3.0f;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionShape"));
	SetRootComponent(CollisionSphere);

	CollisionSphere->InitSphereRadius(10.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	CollisionSphere->SetGenerateOverlapEvents(false);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionSphere);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetGenerateOverlapEvents(false);
	ProjectileMesh->SetCastShadow(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionSphere);
	ProjectileMovement->InitialSpeed = 2200.0f;
	ProjectileMovement->MaxSpeed = 2200.0f;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bInitialVelocityInLocalSpace = true;
	ProjectileMovement->Velocity = FVector::ForwardVector;

	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->bSweepCollision = true;
	ProjectileMovement->OnProjectileStop.AddDynamic(this, &ARGBProjectile::HandleProjectileStopped);
}

void ARGBProjectile::InitializeColor(ERGBColor InColor)
{
	if (bColorInitialized) {
		return;
	}

	UMaterialInterface* SelectedMaterial = nullptr;
	FLinearColor SelectedVFXColor = FLinearColor::White;

	switch (InColor) {
	case ERGBColor::Red:
		SelectedMaterial = RedMaterial.Get();
		SelectedVFXColor = RedVFXColor;
		break;

	case ERGBColor::Green:
		SelectedMaterial = GreenMaterial.Get();
		SelectedVFXColor = GreenVFXColor;
		break;

	case ERGBColor::Blue:
		SelectedMaterial = BlueMaterial.Get();
		SelectedVFXColor = BlueVFXColor;
		break;

	default:
		ensureMsgf(false, TEXT("Invalid projectile color."));
		return;
	}

	ProjectileColor = InColor;
	bColorInitialized = true;

	if (IsValid(ProjectileMesh) && IsValid(SelectedMaterial)) {
		ProjectileMesh->SetMaterial(0, SelectedMaterial);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("%s: missing projectile mesh or color material."), *GetName());
	}
	OnProjectileColorInitialized(SelectedVFXColor);
}

void ARGBProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Shooter = GetOwner()) {
		CollisionSphere->IgnoreActorWhenMoving(Shooter, true);
	}
}

void ARGBProjectile::HandleProjectileStopped(const FHitResult& ImpactResult)
{
	if (IsValid(ImpactSound)) {
		FVector SoundLocation = GetActorLocation();

		if (ImpactResult.bBlockingHit) {
			SoundLocation = FVector(ImpactResult.ImpactPoint);
		}
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, SoundLocation);
	}

	if (bColorInitialized) {
		if (ARGBEnemyCharacter* Enemy = Cast<ARGBEnemyCharacter>(ImpactResult.GetActor())) {
			Enemy->RecieveColorHit(ProjectileColor);
		}
	}
	Destroy();
}