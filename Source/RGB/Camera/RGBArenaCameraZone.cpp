// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBArenaCameraZone.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "../Player/RGBPlayerCharacter.h"


// Sets default values
ARGBArenaCameraZone::ARGBArenaCameraZone()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ArenaBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ArenaBounds"));
	ArenaBounds->SetupAttachment(SceneRoot);
	ArenaBounds->SetBoxExtent(FVector(1500.0f, 300.0f, 1000.0f));

	ArenaBounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ArenaBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	ArenaBounds->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ArenaBounds->SetGenerateOverlapEvents(true);

	ArenaCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ArenaCamera"));
	ArenaCamera->SetupAttachment(SceneRoot);
	ArenaCamera->SetRelativeLocation(FVector(0.0f, 4000.0f, 700.0f));
	ArenaCamera->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	ArenaCamera->ProjectionMode = ECameraProjectionMode::Perspective;
	ArenaCamera->FieldOfView = 60.0f;
	ArenaCamera->bUsePawnControlRotation = false;
}

bool ARGBArenaCameraZone::ContainWorldLocation(const FVector& WorldLocation) const
{
	if (!IsValid(ArenaBounds)) {
		return false;
	}

	const FVector LocalLocation = ArenaBounds->GetComponentTransform().InverseTransformPosition(WorldLocation);
	const FVector Extent = ArenaBounds->GetUnscaledBoxExtent();

	return FMath::Abs(LocalLocation.X) <= Extent.X && FMath::Abs(LocalLocation.Y) <= Extent.Y && FMath::Abs(LocalLocation.Z) <= Extent.Z;
}

void ARGBArenaCameraZone::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(CameraZoneTimer, this, &ARGBArenaCameraZone::UpdateCameraZone, 0.05f, true);
}

void ARGBArenaCameraZone::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(CameraZoneTimer);

	if (bArenaViewActive && EndPlayReason == EEndPlayReason::Destroyed) {
		APlayerController* PlayerController = TrackedController.Get();

		if (IsValid(PlayerController) && IsValid(PlayerController->GetPawn())) {
			PlayerController->SetViewTargetWithBlend(PlayerController->GetPawn(), BlendDuration, VTBlend_EaseInOut, 2.0f, true);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ARGBArenaCameraZone::UpdateCameraZone()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);

	if (!IsValid(PlayerController) || !PlayerController->IsLocalController()) {
		return;
	}

	ARGBPlayerCharacter* Player = Cast<ARGBPlayerCharacter>(PlayerController->GetPawn());

	if (!IsValid(Player)) {
		return;
	}

	const bool bPlayerInside = ArenaBounds->IsOverlappingComponent(Player->GetCapsuleComponent());
	const bool bPlayerChanged = TrackedPawn.Get() != Player || TrackedController.Get() != PlayerController;

	TrackedPawn = Player;
	TrackedController = PlayerController;

	if (bPlayerInside) {
		if (!bArenaViewActive || bPlayerChanged) {
			PlayerController->SetViewTargetWithBlend(this, BlendDuration, VTBlend_EaseInOut, 2.0f, true);
			bArenaViewActive = true;
		}
	}
	else if (bArenaViewActive) {
		PlayerController->SetViewTargetWithBlend(Player, BlendDuration, VTBlend_EaseInOut, 2.0f, true);
		bArenaViewActive = false;
	}
}
