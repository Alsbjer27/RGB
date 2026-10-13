// Fill out your copyright notice in the Description page of Project Settings.

#include "RGBFlyingEnemyCharacter.h"

#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "../Camera/RGBArenaCameraZone.h"
#include "../Player/RGBPlayerCharacter.h"

ARGBFlyingEnemyCharacter::ARGBFlyingEnemyCharacter()
{
	bPatrolEnabled = false;
	PrimaryActorTick.bStartWithTickEnabled = true;
	FlyingBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FlyingBody"));
	FlyingBody->SetupAttachment(GetCapsuleComponent());
	FlyingBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetVisibility(false);

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(48.0f, 48.0f);
	Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Capsule->SetGenerateOverlapEvents(true);

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->GravityScale = 0.0f;
	Movement->MaxFlySpeed = MaximumFlightSpeed;
	Movement->MaxAcceleration = FlightAcceleration;
	Movement->BrakingDecelerationFlying = FlightDeceleration;
	Movement->BrakingFrictionFactor = 1.0f;
	Movement->bOrientRotationToMovement = false;
	Movement->bEnablePhysicsInteraction = false;
}

void ARGBFlyingEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	FlightPlaneY = GetActorLocation().Y;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->MaxFlySpeed = FMath::Max(MaximumFlightSpeed, 0.0f);
	Movement->MaxAcceleration = FMath::Max(FlightAcceleration, 0.0f);
	Movement->BrakingDecelerationFlying = FMath::Max(FlightDeceleration, 0.0f);
	Movement->SetPlaneConstraintEnabled(true);
	Movement->SetPlaneConstraintNormal(FVector::YAxisVector);
	Movement->SetPlaneConstraintOrigin(FVector(0.0f, FlightPlaneY, 0.0f));
	Movement->SetMovementMode(MOVE_Flying);
	Movement->SnapUpdatedComponentToPlane();
	Movement->StopMovementImmediately();
	Movement->SetComponentTickEnabled(false);
	GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	GetMesh()->SetComponentTickEnabled(false);
	SetActorTickEnabled(true);

	if (!IsValid(GetArenaCameraZone())) {
		for (TActorIterator<ARGBArenaCameraZone> It(GetWorld()); It; ++It) {
			if (IsValid(*It) && It->ContainWorldLocation(GetActorLocation())) {
				SetArenaCameraZone(*It);
				break;
			}
		}
	}

	if (!IsValid(GetArenaCameraZone())) {
		UE_LOG(LogTemp, Warning, TEXT("%s is not inside an arena camera zone and cannot be confined to arena bounds."), *GetName());
	}

	RefreshPlayerTarget();

	const float SafeRefreshInterval = FMath::Max(TargetRefreshInterval, 0.01f);
	GetWorldTimerManager().SetTimer(TargetRefreshTimer, this, &ARGBFlyingEnemyCharacter::RefreshPlayerTarget, SafeRefreshInterval, true, SafeRefreshInterval);
}

void ARGBFlyingEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(TargetRefreshTimer);
	FlightVelocity = FVector::ZeroVector;
	FlightTargetPlayer.Reset();
	Super::EndPlay(EndPlayReason);
}

void ARGBFlyingEnemyCharacter::Tick(float DeltaTime)
{
	ACharacter::Tick(DeltaTime);
	if (!IsArenaAIActive()) {
		return;
	}
	UpdateFlight(DeltaTime);
}

void ARGBFlyingEnemyCharacter::ApplyColorMaterial(ERGBColor Color)
{
	Super::ApplyColorMaterial(Color);

	if (!IsValid(FlyingBody) || !IsValid(GetMesh())) {
		return;
	}

	FlyingBody->SetMaterial(0, GetMesh()->GetMaterial(0));
	FlyingBody->SetMaterial(1, GetMesh()->GetMaterial(1));
}

void ARGBFlyingEnemyCharacter::RefreshPlayerTarget()
{
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);

	if (!IsValid(Player)) {
		for (TActorIterator<ARGBPlayerCharacter> It(GetWorld()); It; ++It) {
			if (IsValid(*It)) {
				Player = *It;
				break;
			}
		}
	}

	FlightTargetPlayer = IsValid(Player) && Player != this ? Player : nullptr;

	if (FlightTargetPlayer.IsValid()) {
		bReportedMissingPlayer = false;
	}
	else if (!bReportedMissingPlayer) {
		UE_LOG(LogTemp, Warning, TEXT("%s cannot find a player character to track."), *GetName());
		bReportedMissingPlayer = true;
	}
}

void ARGBFlyingEnemyCharacter::UpdateFlight(float DeltaTime)
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (!IsValid(Movement) || Movement->MovementMode != MOVE_Flying) {
		return;
	}

	if (IsAttackInProgress()) {
		FlightVelocity = FVector::ZeroVector;
		Movement->Velocity = FVector::ZeroVector;
		return;
	}

	ACharacter* Player = FlightTargetPlayer.Get();

	if (!IsValid(Player)) {
		FlightVelocity = FMath::VInterpConstantTo(FlightVelocity, FVector::ZeroVector, DeltaTime, FMath::Max(FlightDeceleration, 0.0f));
		Movement->Velocity = FlightVelocity;
		SetActionState(ERGBEnemyActionState::Idle);
		return;
	}

	FRGBEnemyCoordinationAssignment Assignment;
	const bool bHasAssignment = GetCoordinationAssignment(Assignment);
	const bool bMayAttemptAttack = !HasEnemyCoordinator() || (bHasAssignment && Assignment.bCanAttack);

	if (bMayAttemptAttack) {
		StartAttackAgainst(Player);

		if (IsAttackInProgress()) {
			return;
		}
	}

	FVector ToTarget = Player->GetActorLocation() - GetActorLocation();
	ToTarget.Y = 0.0f;

	const float SafeArrivalRadius = FMath::Max(PlayerArrivalRadius, 0.0f);
	FVector PursuitDirection = ToTarget.SizeSquared() > FMath::Square(SafeArrivalRadius) ? ToTarget.GetSafeNormal() : FVector::ZeroVector;
	const FVector SeparationDirection = CalculateSeparationSteering();
	FVector DesiredDirection = PursuitDirection * FMath::Max(PlayerTrackingWeight, 0.0f) + SeparationDirection * FMath::Max(EnemySeparationWeight, 0.0f);
	DesiredDirection.Y = 0.0f;

	if (DesiredDirection.IsNearlyZero()) {
		FlightVelocity = FMath::VInterpConstantTo(FlightVelocity, FVector::ZeroVector, DeltaTime, FMath::Max(FlightDeceleration, 0.0f));
		Movement->Velocity = FlightVelocity;
		SetActionState(ERGBEnemyActionState::Chase);
		return;
	}

	DesiredDirection.Normalize();
	const FVector ClearDirection = ChooseClearFlightDirection(DesiredDirection);

	if (ClearDirection.IsNearlyZero()) {
		FlightVelocity = FMath::VInterpConstantTo(FlightVelocity, FVector::ZeroVector, DeltaTime, FMath::Max(FlightDeceleration, 0.0f));
		Movement->Velocity = FlightVelocity;
		SetActionState(ERGBEnemyActionState::Chase);
		return;
	}

	SetActionState(ERGBEnemyActionState::Chase);
	const FVector DesiredVelocity = ClearDirection * FMath::Max(MaximumFlightSpeed, 0.0f);
	FlightVelocity = FMath::VInterpConstantTo(FlightVelocity, DesiredVelocity, DeltaTime, FMath::Max(FlightAcceleration, 0.0f));
	FlightVelocity.Y = 0.0f;

	const FVector StartLocation = GetActorLocation();
	const FVector MoveDelta = ConstrainMoveDeltaToArena(StartLocation, FlightVelocity * FMath::Max(DeltaTime, 0.0f));
	FHitResult Hit;
	Movement->SafeMoveUpdatedComponent(MoveDelta, GetActorQuat(), true, Hit);

	if (Hit.IsValidBlockingHit()) {
		const FVector SlideDelta = ConstrainMoveDeltaToArena(GetActorLocation(), FVector::VectorPlaneProject(MoveDelta * (1.0f - Hit.Time), Hit.Normal));
		FHitResult SlideHit;
		Movement->SafeMoveUpdatedComponent(SlideDelta, GetActorQuat(), true, SlideHit);
		FlightVelocity = FVector::VectorPlaneProject(FlightVelocity, Hit.Normal);
		FlightVelocity.Y = 0.0f;
	}

	Movement->Velocity = FlightVelocity;
	Movement->UpdateComponentVelocity();

	const float HorizontalTargetDistance = FMath::Abs(ToTarget.X);
	const float HorizontalMoveDistance = FMath::Abs(GetActorLocation().X - StartLocation.X);
	if (HorizontalTargetDistance > 150.0f && HorizontalMoveDistance < 0.5f) {
		HorizontalBlockDuration += DeltaTime;
	}
	else {
		HorizontalBlockDuration = 0.0f;
	}

	if (HorizontalBlockDuration >= 1.0f && GetWorld()->GetTimeSeconds() >= NextHorizontalBlockLogTime) {
		FHitResult ProbeHit;
		const float ForwardClearance = MeasureObstacleClearance(PursuitDirection, &ProbeHit);
		UE_LOG(LogTemp, Warning, TEXT("%s cannot make horizontal progress toward %s. PlayerDelta=%s FlightDirection=%s Clearance=%.2f ProbeHit=%s MoveHit=%s"), *GetName(), *GetNameSafe(Player), *ToTarget.ToCompactString(), *ClearDirection.ToCompactString(), ForwardClearance, *GetNameSafe(ProbeHit.GetActor()), *GetNameSafe(Hit.GetActor()));
		NextHorizontalBlockLogTime = GetWorld()->GetTimeSeconds() + 3.0f;
	}

	UpdateVisualFacing(ClearDirection, DeltaTime);
}

bool ARGBFlyingEnemyCharacter::IsPlayerWithinAttackRange(const ACharacter* Player) const
{
	if (!IsValid(Player) || !IsValid(Player->GetCapsuleComponent())) {
		return false;
	}

	const UCapsuleComponent* PlayerCapsule = Player->GetCapsuleComponent();
	const float PlayerRadius = PlayerCapsule->GetScaledCapsuleRadius();
	const float PlayerSegmentHalfHeight = FMath::Max(PlayerCapsule->GetScaledCapsuleHalfHeight() - PlayerRadius, 0.0f);
	const FVector PlayerLocation = Player->GetActorLocation();
	const FVector EnemyLocation = GetActorLocation();
	const float ClosestPlayerZ = FMath::Clamp(EnemyLocation.Z, PlayerLocation.Z - PlayerSegmentHalfHeight, PlayerLocation.Z + PlayerSegmentHalfHeight);
	const float DeltaX = EnemyLocation.X - PlayerLocation.X;
	const float DeltaZ = EnemyLocation.Z - ClosestPlayerZ;
	const float AllowedDistance = GetCapsuleComponent()->GetScaledCapsuleRadius() + PlayerRadius + FMath::Max(StopDistanceFromPlayer, 0.0f) + FMath::Max(AttackHitRangePadding, 0.0f);

	return DeltaX * DeltaX + DeltaZ * DeltaZ <= FMath::Square(AllowedDistance);
}

void ARGBFlyingEnemyCharacter::HandleArenaAIActiveChanged(bool bNewActive)
{
	FlightVelocity = FVector::ZeroVector;
	HorizontalBlockDuration = 0.0f;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement()) {
		Movement->Velocity = FVector::ZeroVector;
		Movement->UpdateComponentVelocity();
	}

	if (bNewActive) {
		RefreshPlayerTarget();
	}
}

FVector ARGBFlyingEnemyCharacter::CalculateSeparationSteering() const
{
	const UWorld* World = GetWorld();
	const float SafeSeparationRadius = FMath::Max(EnemySeparationRadius, 0.0f);

	if (!IsValid(World) || SafeSeparationRadius <= SMALL_NUMBER) {
		return FVector::ZeroVector;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjectQuery;
	ObjectQuery.AddObjectTypesToQuery(ECC_Pawn);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, GetActorLocation(), FQuat::Identity, ObjectQuery, FCollisionShape::MakeSphere(SafeSeparationRadius), QueryParams);

	FVector Separation = FVector::ZeroVector;

	for (const FOverlapResult& Overlap : Overlaps) {
		const ARGBEnemyCharacter* OtherEnemy = Cast<ARGBEnemyCharacter>(Overlap.GetActor());

		if (!IsValid(OtherEnemy) || OtherEnemy == this) {
			continue;
		}

		FVector Away = GetActorLocation() - OtherEnemy->GetActorLocation();
		Away.Y = 0.0f;
		const float Distance = Away.Size();

		if (Distance <= SMALL_NUMBER) {
			const float Side = GetUniqueID() < OtherEnemy->GetUniqueID() ? -1.0f : 1.0f;
			Separation += FVector(Side, 0.0f, 0.0f);
			continue;
		}

		const float Strength = 1.0f - FMath::Clamp(Distance / SafeSeparationRadius, 0.0f, 1.0f);
		Separation += Away / Distance * Strength;
	}

	return Separation.GetClampedToMaxSize(1.0f);
}

FVector ARGBFlyingEnemyCharacter::ChooseClearFlightDirection(const FVector& DesiredDirection) const
{
	if (DesiredDirection.IsNearlyZero()) {
		return FVector::ZeroVector;
	}

	const float DesiredClearance = MeasureObstacleClearance(DesiredDirection);

	if (DesiredClearance >= 1.0f - KINDA_SMALL_NUMBER) {
		return DesiredDirection;
	}

	const FVector Perpendicular(-DesiredDirection.Z, 0.0f, DesiredDirection.X);
	const FVector CandidateDirections[] = {
		(DesiredDirection + Perpendicular * FMath::Max(ObstacleAvoidanceWeight, 0.0f)).GetSafeNormal(),
		(DesiredDirection - Perpendicular * FMath::Max(ObstacleAvoidanceWeight, 0.0f)).GetSafeNormal(),
		Perpendicular,
		-Perpendicular,
		FVector::UpVector,
		FVector::DownVector
	};

	FVector BestDirection = FVector::ZeroVector;
	float BestScore = DesiredClearance + 0.25f;

	for (const FVector& CandidateDirection : CandidateDirections) {
		const float Clearance = MeasureObstacleClearance(CandidateDirection);
		const float Alignment = FVector::DotProduct(CandidateDirection, DesiredDirection);
		const float Score = Clearance * 2.0f + Alignment * 0.35f;

		if (Score > BestScore) {
			BestScore = Score;
			BestDirection = CandidateDirection;
		}
	}

	return BestDirection;
}

float ARGBFlyingEnemyCharacter::MeasureObstacleClearance(const FVector& Direction, FHitResult* OutHit) const
{
	const UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const float SafeProbeDistance = FMath::Max(ObstacleProbeDistance, 0.0f);

	if (!IsValid(World) || !IsValid(Capsule) || Direction.IsNearlyZero() || SafeProbeDistance <= SMALL_NUMBER) {
		return 1.0f;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const float Radius = Capsule->GetScaledCapsuleRadius() * FMath::Clamp(ObstacleProbeRadiusScale, 0.0f, 1.0f);
	const FVector Start = GetActorLocation();
	const FVector End = Start + Direction.GetSafeNormal() * SafeProbeDistance;
	FHitResult Hit;
	const bool bBlocked = World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(Radius), QueryParams);
	if (OutHit) {
		*OutHit = Hit;
	}
	return bBlocked ? FMath::Clamp(Hit.Time, 0.0f, 1.0f) : 1.0f;
}

FVector ARGBFlyingEnemyCharacter::ConstrainMoveDeltaToArena(const FVector& StartLocation, const FVector& DesiredDelta) const
{
	const ARGBArenaCameraZone* CameraZone = GetArenaCameraZone();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();

	if (!IsValid(CameraZone) || !IsValid(Capsule)) {
		return DesiredDelta;
	}

	const FVector Padding(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight());
	FVector ClampedTarget = CameraZone->ClampWorldLocation(StartLocation + DesiredDelta, Padding);
	ClampedTarget.Y = FlightPlaneY;
	return ClampedTarget - StartLocation;
}

void ARGBFlyingEnemyCharacter::UpdateVisualFacing(const FVector& FlightDirection, float DeltaTime)
{
	if (!bOrientVisualToFlightDirection || FMath::IsNearlyZero(FlightDirection.X)) {
		return;
	}

	const float DesiredYaw = FlightDirection.X >= 0.0f ? 0.0f : 180.0f;
	const FRotator DesiredRotation(0.0f, DesiredYaw, 0.0f);
	SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(), DesiredRotation, DeltaTime, FMath::Max(VisualRotationSpeed, 0.0f)));
}
