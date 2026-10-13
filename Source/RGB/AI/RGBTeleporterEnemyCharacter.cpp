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

#include "EngineUtils.h"

ARGBTeleporterEnemyCharacter::ARGBTeleporterEnemyCharacter()
{
	bPatrolEnabled = false;

	AoEWarningEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("AoEWarning"));
	AoEWarningEffect->SetupAttachment(GetRootComponent());
	AoEWarningEffect->SetAutoActivate(false);

	TeleportChargeEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TeleportCharge"));
	TeleportChargeEffect->SetupAttachment(GetMesh());
	TeleportChargeEffect->SetAbsolute(false, false, true);
	TeleportChargeEffect->SetAutoActivate(false);
}

void ARGBTeleporterEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateTeleportCharge(DeltaTime);
}

void ARGBTeleporterEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultMeshRelativeScale = GetMesh()->GetRelativeScale3D();
	bHasCapturedDefaultMeshScale = true;
	FindTeleportPoints();

	if (TeleportPoints.IsEmpty()) {
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s found no nearby Target Points with tag %s."),
			*GetName(),
			*TeleportPointTag.ToString());

	}
	else if (IsArenaAIActive()) {
		ScheduleNextTeleport();
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
	ReleaseCoordinatedAttackPermission();
	ReleaseCoordinationTarget(PendingTeleportPoint.Get());

	TeleportPoints.Reset();
	LastTeleportPoint.Reset();

	if (IsValid(TeleportChargeEffect))
	{
		TeleportChargeEffect->Deactivate();
	}

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
	if (!IsArenaAIActive())
	{
		return;
	}

	if (bAoEWindInProgress || bTeleportInProgress)
	{
		ScheduleNextTeleport();
		return;
	}

	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);

	if (!IsValid(Player))
	{
		ScheduleNextTeleport();
		return;
	}

	bool bPlayerNearTeleportPoint = false;
	const float SafeDetectionRange = FMath::Max(DetectionRange, 0.0f);

	for (const TWeakObjectPtr<ATargetPoint>& PointPointer : TeleportPoints)
	{
		const ATargetPoint* Point = PointPointer.Get();

		if (!IsValid(Point))
		{
			continue;
		}

		FVector DifferenceToPoint = Player->GetActorLocation() - Point->GetActorLocation();
		DifferenceToPoint.Y = 0.0f;

		if (DifferenceToPoint.SizeSquared() <= FMath::Square(SafeDetectionRange))
		{
			bPlayerNearTeleportPoint = true;
			break;
		}
	}

	if (!bPlayerNearTeleportPoint)
	{
		ScheduleNextTeleport();
		return;
	}

	TArray<ATargetPoint*> ValidPoints;
	TArray<float> CandidateDistancesToPlayer;

	for (const TWeakObjectPtr<ATargetPoint>& PointPointer : TeleportPoints)
	{
		ATargetPoint* Point = PointPointer.Get();

		if (!IsValid(Point) || Point == LastTeleportPoint.Get())
		{
			continue;
		}

		if (IsTeleportPointReservedByOther(Point))
		{
			continue;
		}

		FVector Destination = Point->GetActorLocation();
		Destination.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Destination.Y = GetActorLocation().Y;

		if (!IsTeleportDestinationClear(Destination))
		{
			continue;
		}

		FVector DifferenceFromCurrent = Destination - GetActorLocation();
		DifferenceFromCurrent.Y = 0.0f;

		if (DifferenceFromCurrent.SizeSquared() < FMath::Square(FMath::Max(MinimumTeleportDistance, 0.0f)))
		{
			continue;
		}

		ValidPoints.Add(Point);

		FVector DifferenceFromPlayer = Destination - Player->GetActorLocation();
		DifferenceFromPlayer.Y = 0.0f;
		CandidateDistancesToPlayer.Add(DifferenceFromPlayer.Size());
	}

	if (ValidPoints.IsEmpty())
	{
		ScheduleNextTeleport();
		return;
	}

	const float SafePlayerProximityWeight = FMath::Clamp(PlayerProximityWeight, 0.0f, 1.0f);
	const float MinimumCandidateDistance = FMath::Min(CandidateDistancesToPlayer);
	const float MaximumCandidateDistance = FMath::Max(CandidateDistancesToPlayer);

	int32 SelectedPointIndex = 0;
	float BestScore = -1.0f;

	for (int32 Index = 0; Index < ValidPoints.Num(); ++Index)
	{
		const float DistanceRange = MaximumCandidateDistance - MinimumCandidateDistance;
		const float NormalizedDistance = DistanceRange > SMALL_NUMBER ? (CandidateDistancesToPlayer[Index] - MinimumCandidateDistance) / DistanceRange : 0.0f;
		const float PlayerClosenessScore = 1.0f - NormalizedDistance;
		const float RandomScore = FMath::FRand();
		const float FinalScore = FMath::Lerp(RandomScore, PlayerClosenessScore, SafePlayerProximityWeight);

		if (FinalScore > BestScore)
		{
			BestScore = FinalScore;
			SelectedPointIndex = Index;
		}
	}

	ATargetPoint* SelectedPoint = ValidPoints[SelectedPointIndex];

	if (!TryReserveCoordinationTarget(SelectedPoint)) {
		ScheduleNextTeleport();
		return;
	}

	FVector Destination = SelectedPoint->GetActorLocation();
	Destination.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Destination.Y = GetActorLocation().Y;

	const FRotator DestinationRotation = GetActorRotation();

	BeginTeleport(Destination, DestinationRotation, SelectedPoint);
}

void ARGBTeleporterEnemyCharacter::BeginTeleport(const FVector& Destination, const FRotator& DestinationRotation, ATargetPoint* DestiantionPoint)
{
	if (bTeleportInProgress) {
		return;
	}

	bTeleportInProgress = true;
	SetActionState(ERGBEnemyActionState::TeleportCharge);

	TeleportChargeElapsed = 0.0f;
	GetMesh()->SetRelativeScale3D(DefaultMeshRelativeScale);

	if (IsValid(TeleportChargeEffect))
	{
		TeleportChargeEffect->SetVariableFloat(TeleportChargeParameterName, 0.0f);
		TeleportChargeEffect->Activate(true);
	}

	PendingTeleportDestiantion = Destination;
	PendingTeleportRotation = DestinationRotation;
	PendingTeleportPoint = DestiantionPoint;



	const float SafeDelay = FMath::Max(TeleportEffectDelay, 0.0f);

	if (SafeDelay <= SMALL_NUMBER) {
		CompleteTeleport();
		return;
	}

	GetWorldTimerManager().SetTimer(TeleportTransistionTimer, this, &ARGBTeleporterEnemyCharacter::CompleteTeleport, SafeDelay, false);
}

void ARGBTeleporterEnemyCharacter::CompleteTeleport()
{
	ATargetPoint* DestinationPoint = PendingTeleportPoint.Get();

	if (!IsValid(DestinationPoint) || IsTeleportPointReservedByOther(DestinationPoint))
	{
		if (IsValid(TeleportChargeEffect))
		{
			TeleportChargeEffect->Deactivate();
		}

		GetMesh()->SetRelativeScale3D(DefaultMeshRelativeScale);
		ReleaseCoordinationTarget(DestinationPoint);
		PendingTeleportPoint.Reset();
		bTeleportInProgress = false;
		SetActionState(ERGBEnemyActionState::Idle);
		ScheduleNextTeleport();
		return;
	}

	FVector RevalidatedDestination = DestinationPoint->GetActorLocation();
	RevalidatedDestination.Z += GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	RevalidatedDestination.Y = GetActorLocation().Y;

	if (!IsTeleportDestinationClear(RevalidatedDestination))
	{
		if (IsValid(TeleportChargeEffect))
		{
			TeleportChargeEffect->Deactivate();
		}

		GetMesh()->SetRelativeScale3D(DefaultMeshRelativeScale);
		ReleaseCoordinationTarget(DestinationPoint);
		PendingTeleportPoint.Reset();
		bTeleportInProgress = false;
		SetActionState(ERGBEnemyActionState::Idle);
		ScheduleNextTeleport();
		return;
	}

	if (IsValid(TeleportChargeEffect))
	{
		TeleportChargeEffect->SetVariableFloat(TeleportChargeParameterName, 1.0f);
		TeleportChargeEffect->Deactivate();
	}

	SpawnTeleportEffect(TeleportDepartureEffect, GetActorLocation());
	const bool bTeleported = TeleportTo(RevalidatedDestination, PendingTeleportRotation, false, false);
	GetMesh()->SetRelativeScale3D(DefaultMeshRelativeScale);

	if (bTeleported) {
		LastTeleportPoint = PendingTeleportPoint;
		SpawnTeleportEffect(TeleportArrivalEffect, GetActorLocation());
	}

	ReleaseCoordinationTarget(DestinationPoint);
	PendingTeleportPoint.Reset();
	bTeleportInProgress = false;
	SetActionState(ERGBEnemyActionState::Idle);
	ScheduleNextTeleport();
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
	if (!IsArenaAIActive() || bAoEWindInProgress || bTeleportInProgress || GetWorld()->GetTimeSeconds() < NextAoEAttackAllowedTime) {
		return;
	}

	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);

	if (!IsPlayerWithinAoERadius(Player)) {
		return;
	}

	if (!TryAcquireCoordinatedAttackPermission()) {
		return;
	}

	bAoEWindInProgress = true;
	SetActionState(ERGBEnemyActionState::AoEWindup);
	SetAoEWarningActive(true);

	const float SafeWindupDuration = FMath::Max(AoEWindupDuration, 0.0f);

	if (SafeWindupDuration <= SMALL_NUMBER) {
		ResolveAoEAttack();
		return;
	}

	GetWorldTimerManager().SetTimer(AoEWindupTimer, this, &ARGBTeleporterEnemyCharacter::ResolveAoEAttack, SafeWindupDuration, false);
}

void ARGBTeleporterEnemyCharacter::HandleArenaAIActiveChanged(bool bNewActive)
{
	GetWorldTimerManager().ClearTimer(TeleportTimer);
	GetWorldTimerManager().ClearTimer(AoEWindupTimer);
	GetWorldTimerManager().ClearTimer(TeleportTransistionTimer);
	bAoEWindInProgress = false;
	bTeleportInProgress = false;
	TeleportChargeElapsed = 0.0f;
	SetAoEWarningActive(false);
	ReleaseCoordinationTarget(PendingTeleportPoint.Get());
	PendingTeleportPoint.Reset();

	if (IsValid(TeleportChargeEffect)) {
		TeleportChargeEffect->Deactivate();
	}

	if (bHasCapturedDefaultMeshScale && IsValid(GetMesh())) {
		GetMesh()->SetRelativeScale3D(DefaultMeshRelativeScale);
	}

	if (bNewActive && !TeleportPoints.IsEmpty()) {
		ScheduleNextTeleport();
	}
}

void ARGBTeleporterEnemyCharacter::ResolveAoEAttack()
{
	bAoEWindInProgress = false;
	SetAoEWarningActive(false);
	ReleaseCoordinatedAttackPermission();
	SetActionState(ERGBEnemyActionState::Idle);

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

void ARGBTeleporterEnemyCharacter::ScheduleNextTeleport()
{
	const float SafeMinimumIdleTime = FMath::Max(MinimumTeleportIdleTime, 0.1f);
	const float SafeMaximumIdleTime = FMath::Max(MaximumTeleportIdleTime, SafeMinimumIdleTime);
	const float NextTeleportDelay = FMath::FRandRange(SafeMinimumIdleTime, SafeMaximumIdleTime);

	GetWorldTimerManager().SetTimer(TeleportTimer, this, &ARGBTeleporterEnemyCharacter::TryTeleport, NextTeleportDelay, false);
}

bool ARGBTeleporterEnemyCharacter::IsTeleportPointReservedByOther(const ATargetPoint* Point) const
{
	if (!IsValid(Point))
	{
		return true;
	}

	if (HasEnemyCoordinator())
	{
		return IsCoordinationTargetReservedByOther(Point);
	}

	for (TActorIterator<ARGBTeleporterEnemyCharacter> It(GetWorld()); It; ++It)
	{
		const ARGBTeleporterEnemyCharacter* OtherTeleporter = *It;

		if (!IsValid(OtherTeleporter) || OtherTeleporter == this)
		{
			continue;
		}

		if (OtherTeleporter->PendingTeleportPoint.Get() == Point)
		{
			return true;
		}
	}

	return false;
}

void ARGBTeleporterEnemyCharacter::UpdateTeleportCharge(float DeltaTime)
{
	if (!bTeleportInProgress || !IsValid(TeleportChargeEffect))
	{
		return;
	}

	TeleportChargeElapsed += FMath::Max(DeltaTime, 0.0f);

	const float SafeChargeDuration = FMath::Max(TeleportEffectDelay, 0.01f);
	const float NormalizedCharge = FMath::Clamp(TeleportChargeElapsed / SafeChargeDuration, 0.0f, 1.0f);
	const float SafeChargeExponent = FMath::Max(TeleportChargeExponent, 0.1f);
	const float ShapedCharge = FMath::Pow(NormalizedCharge, SafeChargeExponent);

	TeleportChargeEffect->SetVariableFloat(TeleportChargeParameterName, ShapedCharge);

	const float SafeImplosionStartCharge = FMath::Clamp(ImplosionStartCharge, 0.0f, 0.99f);
	const float ImplosionAlpha = FMath::GetMappedRangeValueClamped(FVector2D(SafeImplosionStartCharge, 1.0f), FVector2D(0.0f, 1.0f), ShapedCharge);
	const float ShapedImplosionAlpha = FMath::Pow(ImplosionAlpha, FMath::Max(ImplosionExponent, 0.1f));
	const float ImplosionScale = FMath::Lerp(1.0f, ImplosionMinimumScale, ShapedImplosionAlpha);
	GetMesh()->SetRelativeScale3D(DefaultMeshRelativeScale * ImplosionScale);
}
