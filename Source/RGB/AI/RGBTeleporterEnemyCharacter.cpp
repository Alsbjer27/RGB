// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBTeleporterEnemyCharacter.h"

#include "Components/CapsuleComponent.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/World.h"

#include "../Player/RGBPlayerCharacter.h"

#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"

ARGBTeleporterEnemyCharacter::ARGBTeleporterEnemyCharacter()
{
	bPatrolEnabled = false;

	AoEWarningEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("AoEWarning"));
	AoEWarningEffect->SetupAttachment(GetRootComponent());
	AoEWarningEffect->SetAutoActivate(false);
}

void ARGBTeleporterEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	FindTeleportPoints();

	if (TeleportPoints.IsEmpty()) {
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s found no nearby Target Points with tag %s."),
			*GetName(),
			*TeleportPointTag.ToString());

	}
	else {
		const float SafeInterval = FMath::Max(TeleportInterval, 0.1f);

		GetWorldTimerManager().SetTimer(TeleportTimer, this, &ARGBTeleporterEnemyCharacter::TryTeleport, SafeInterval, true, SafeInterval);
	}
	
	const float SafeAoECheckInterval = FMath::Max(AoECheckInterval, 0.01f);
	GetWorldTimerManager().SetTimer(AoECheckTimer, this, &ARGBTeleporterEnemyCharacter::TryStartAoEAttack, SafeAoECheckInterval, true, SafeAoECheckInterval);

	AoEWarningEffect->SetRelativeLocation(FVector(0.0f, 0.0f, -GetCapsuleComponent()->GetScaledCapsuleHalfHeight())+ AoEWarningOffset);	
	SetAoEWarningActive(false);
}

void ARGBTeleporterEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(TeleportTimer);
	GetWorldTimerManager().ClearTimer(AoECheckTimer);
	GetWorldTimerManager().ClearTimer(AoEWindupTimer);
	GetWorldTimerManager().ClearTimer(TeleportTransistionTimer);

	SetAoEWarningActive(false);

	TeleportPoints.Reset();
	LastTeleportPoint.Reset();

	Super::EndPlay(EndPlayReason);
}

void ARGBTeleporterEnemyCharacter::FindTeleportPoints()
{
	TeleportPoints.Reset();
	LastTeleportPoint.Reset();

	TArray<AActor*> FoundActors;

	UGameplayStatics::GetAllActorsOfClassWithTag(this, ATargetPoint::StaticClass(), TeleportPointTag, FoundActors);

	const FVector StartLocation = GetActorLocation();
	const float SafeSearchRadius = FMath::Max(TeleportPointSearchRadius, 0.0f);

	for (AActor* Actor : FoundActors) {
		ATargetPoint* Point = Cast<ATargetPoint>(Actor);

		if (!IsValid(Point)) {
			continue;
		}

		FVector Difference = Point->GetActorLocation() - StartLocation;
		Difference.Y = 0.0f;

		if (Difference.SizeSquared() <= FMath::Square(SafeSearchRadius)) {
			TeleportPoints.Add(Point);
		}
	}
}

void ARGBTeleporterEnemyCharacter::TryTeleport()
{
	if (bAoEWindInProgress || bTeleportInProgress) {
		return;
	}

	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);

	if (!IsValid(Player)) {
		return;
	}

	bool bPlayerNearTeleportPoint = false;

	const float SafeDetectionRange =
		FMath::Max(DetectionRange, 0.0f);

	for (const TWeakObjectPtr<ATargetPoint>& PointPointer : TeleportPoints) {
		const ATargetPoint* Point = PointPointer.Get();

		if (!IsValid(Point)) {
			continue;
		}

		FVector DifferenceToPoint =
			Player->GetActorLocation() - Point->GetActorLocation();

		DifferenceToPoint.Y = 0.0f;

		if (DifferenceToPoint.SizeSquared()
			<= FMath::Square(SafeDetectionRange))
		{
			bPlayerNearTeleportPoint = true;
			break;
		}
	}

	if (!bPlayerNearTeleportPoint) {
		return;
	}

	ATargetPoint* BestPoint = nullptr;
	float BestDistanceToPlayerSquared = TNumericLimits<float>::Max();

	for (const TWeakObjectPtr<ATargetPoint>& PointPointer : TeleportPoints) {
		ATargetPoint* Point = PointPointer.Get();

		if (!IsValid(Point) || Point == LastTeleportPoint.Get()) {
			continue;
		}

		FVector Destination = Point->GetActorLocation();

		// Target Points are placed on the platform surface.
		Destination.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		// Preserve the 2.5D depth plane.
		Destination.Y = GetActorLocation().Y;

		if (!IsTeleportDestinationClear(Destination)) {
			continue;
		}

		FVector DifferenceFromCurrent = Destination - GetActorLocation();

		DifferenceFromCurrent.Y = 0.0f;

		if (DifferenceFromCurrent.SizeSquared() < FMath::Square(FMath::Max(MinimumTeleportDistance, 0.0f)))
		{
			continue;
		}

		FVector DifferenceFromPlayer = Destination - Player->GetActorLocation();

		DifferenceFromPlayer.Y = 0.0f;

		const float DistanceToPlayerSquared = DifferenceFromPlayer.SizeSquared();

		if (DistanceToPlayerSquared < BestDistanceToPlayerSquared) {
			BestDistanceToPlayerSquared = DistanceToPlayerSquared;
			BestPoint = Point;
		}
	}

	if (!IsValid(BestPoint)) {
		return;
	}

	FVector Destination = BestPoint->GetActorLocation();
	Destination.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Destination.Y = GetActorLocation().Y;

	const FRotator DestinationRotation = GetActorRotation();

	BeginTeleport(Destination, DestinationRotation, BestPoint);
}

void ARGBTeleporterEnemyCharacter::BeginTeleport(const FVector& Destination, const FRotator& DestinationRotation, ATargetPoint* DestiantionPoint)
{
	if (bTeleportInProgress) {
		return;
	}

	bTeleportInProgress = true;

	PendingTeleportDestiantion = Destination;
	PendingTeleportRotation = DestinationRotation;
	PendingTeleportPoint = DestiantionPoint;

	SpawnTeleportEffect(TeleportDepartureEffect, GetActorLocation());

	const float SafeDelay = FMath::Max(TeleportEffectDelay, 0.0f);

	if (SafeDelay <= SMALL_NUMBER) {
		CompleteTeleport();
		return;
	}

	GetWorldTimerManager().SetTimer(TeleportTransistionTimer, this, &ARGBTeleporterEnemyCharacter::CompleteTeleport, SafeDelay, false);
}

void ARGBTeleporterEnemyCharacter::CompleteTeleport()
{
	const bool bTeleported = TeleportTo(PendingTeleportDestiantion, PendingTeleportRotation, false, false);

	if (bTeleported) {
		LastTeleportPoint = PendingTeleportPoint;
		SpawnTeleportEffect(TeleportArrivalEffect, GetActorLocation());
	}

	PendingTeleportPoint.Reset();
	bTeleportInProgress = false;
}

void ARGBTeleporterEnemyCharacter::SpawnTeleportEffect(UNiagaraSystem* Effect, const FVector& Location) const
{
	if (!IsValid(Effect)) {
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Effect, Location);
}

void ARGBTeleporterEnemyCharacter::TryStartAoEAttack()
{
	if (bAoEWindInProgress || bTeleportInProgress || GetWorld()->GetTimeSeconds() < NextAoEAttackAllowedTime) {
		return;
	}

	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);

	if (!IsPlayerWithinAoERadius(Player)) {
		return;
	}

	bAoEWindInProgress = true;
	SetAoEWarningActive(true);

	const float SafeWindupDuration = FMath::Max(AoEWindupDuration, 0.0f);

	if (SafeWindupDuration <= SMALL_NUMBER) {
		ResolveAoEAttack();
		return;
	}

	GetWorldTimerManager().SetTimer(AoEWindupTimer, this, &ARGBTeleporterEnemyCharacter::ResolveAoEAttack, SafeWindupDuration, false);
}

void ARGBTeleporterEnemyCharacter::ResolveAoEAttack()
{
	bAoEWindInProgress = false;
	SetAoEWarningActive(false);

	ARGBPlayerCharacter* Player = Cast<ARGBPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));

	if (IsValid(Player) && AoEDamage > 0 && IsPlayerWithinAoERadius(Player)) {
		Player->RecieveDamage(AoEDamage);
	}

	NextAoEAttackAllowedTime = GetWorld()->GetTimeSeconds() + FMath::Max(AoECooldown, 0.0f);
}

bool ARGBTeleporterEnemyCharacter::IsPlayerWithinAoERadius(const ACharacter* Player) const
{
	if (!IsValid(Player) || AoERadius <= 0.0f) {
		return false;
	}

	FVector Difference = Player->GetActorLocation() - GetActorLocation();
	Difference.Y = 0.0f;

	return Difference.SizeSquared() <= FMath::Square(AoERadius);
}

void ARGBTeleporterEnemyCharacter::SetAoEWarningActive(bool bActive)
{
	if (!IsValid(AoEWarningEffect)) {
		return;
	}

	if (bActive) {
		AoEWarningEffect->Activate(true);
	}
	else {
		AoEWarningEffect->Deactivate();
	}
}

bool ARGBTeleporterEnemyCharacter::IsTeleportDestinationClear(const FVector& Destination) const
{
	const UWorld* World = GetWorld();

	if (!IsValid(World)) {
		return false;
	}

	const float Padding = FMath::Max(TeleportOccupancyPadding, 0.0f);
	const float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius() + Padding;
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + Padding;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const FCollisionObjectQueryParams ObjectQuery(ECC_Pawn);

	return !World->OverlapAnyTestByObjectType(Destination, FQuat::Identity, ObjectQuery, FCollisionShape::MakeCapsule(Radius, HalfHeight), QueryParams);
}
