// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBEnemyCharacter.h"
#include "RGBArenaEnemyCoordinatorComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"

#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include "Materials/MaterialInterface.h"

#include "../Platforms/RGBColorPlatform.h"

#include "../Player/RGBPlayerCharacter.h"
#include "../Arena/RGBArenaControl.h"

// Sets default values
ARGBEnemyCharacter::ARGBEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ColorComponent = CreateDefaultSubobject<URGBColorComponent>(TEXT("ColorComponent"));

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetSimulatePhysics(false);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	Movement->SetPlaneConstraintNormal(FVector::YAxisVector);
	Movement->SetPlaneConstraintOrigin(FVector::ZeroVector);
	Movement->SetPlaneConstraintEnabled(true);
	Movement->bSnapToPlaneAtStart = true;

	Movement->bRunPhysicsWithNoController = true;
	Movement->bOrientRotationToMovement = false;
	
	Movement->MaxWalkSpeed = 200.0f;
	Movement->MaxAcceleration = 2000.0f;
	Movement->BrakingDecelerationWalking = 2000.0f;
	Movement->GravityScale = 3.0f;
	
	Movement->bUseFlatBaseForFloorChecks = true;
	Movement->bCanWalkOffLedges = false;
}

bool ARGBEnemyCharacter::HasEnemyCoordinator() const
{
	return IsValid(EnemyCoordinator.Get());
}

void ARGBEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (!bArenaAIActive || bEliminated) {
		ConsumeMovementInputVector();
		Movement->StopMovementImmediately();
		SetActionState(ERGBEnemyActionState::Idle);
		return;
	}

	if (bAttacking) {
		ConsumeMovementInputVector();

		if (Movement->IsMovingOnGround()) {
			Movement->StopMovementImmediately();
		}

		return;
	}

	if (bTurning) {
		SetActionState(ERGBEnemyActionState::Turning);
		ConsumeMovementInputVector();

		if (Movement->IsMovingOnGround()) {
			Movement->StopMovementImmediately();
		}

		UpdateTurn(DeltaTime);
		return;
	}

	if (!bPatrolEnabled) {
		SetActionState(ERGBEnemyActionState::Idle);
		if (Movement->IsMovingOnGround()) {
			Movement->StopMovementImmediately();
		}
		return;
	}

	if (!Movement->IsMovingOnGround()) {
		SetActionState(ERGBEnemyActionState::Idle);
		return;
	}

	if (UpdateCoordinatedMovement(DeltaTime)) {
		return;
	}

	if (ACharacter* Player = TargetPlayer.Get(); Player && (!ArenaAttributes.bEnabled || !HasEnemyCoordinator())) {
		UpdateGroundMovementSpeed(true);
		SetActionState(ERGBEnemyActionState::Chase);
		const float DifferenceX =
			Player->GetActorLocation().X - GetActorLocation().X;

		if (FMath::Abs(DifferenceX) > 5.0f) {
			const float DesiredDirection = DifferenceX > 0.0f ? 1.0f : -1.0f;

			if (DesiredDirection != PatrolDirection) {
				StartTurn(DesiredDirection);
				return;
			}
		}

		const float RequiredSeparation = GetCapsuleComponent()->GetScaledCapsuleRadius() + Player->GetCapsuleComponent()->GetScaledCapsuleRadius() + FMath::Max(StopDistanceFromPlayer, 0.0f);

		if (FMath::Abs(DifferenceX) <= RequiredSeparation) {
			Movement->StopMovementImmediately();
			ConsumeMovementInputVector();

			StartAttack();
			return;
		}

		if (!CanWalkInDirection(PatrolDirection, DeltaTime)) {
			Movement->StopMovementImmediately();
			return;
		}

		AddMovementInput(FVector::ForwardVector, PatrolDirection);
		return;
	}
	SetActionState(ERGBEnemyActionState::Patrol);
	UpdateGroundMovementSpeed(false);
	if (ArenaAttributes.bEnabled && GetWorld()->GetTimeSeconds() >= NextPatrolDirectionTime) {
		NextPatrolDirectionTime = GetWorld()->GetTimeSeconds() + FMath::FRandRange(ArenaAttributes.MinimumPatrolDirectionTime, ArenaAttributes.MaximumPatrolDirectionTime);
		const float DesiredDirection = FMath::RandBool() ? 1.0f : -1.0f;
		if (DesiredDirection != PatrolDirection && CanWalkInDirection(DesiredDirection, DeltaTime)) {
			StartTurn(DesiredDirection);
			return;
		}
	}

	if (!CanWalkInDirection(PatrolDirection, DeltaTime)) {
		Movement->StopMovementImmediately();

		const float OppositeDirection = -PatrolDirection;

		if (CanWalkInDirection(OppositeDirection, DeltaTime)) {
			StartTurn(OppositeDirection);
		}
		return;
	}
	AddMovementInput(FVector::ForwardVector, PatrolDirection);
}

bool ARGBEnemyCharacter::RecieveColorHit(ERGBColor ProjectileColor)
{
	if (bEliminated || !IsValid(ColorComponent)) {
		return false;
	}

	if (ProjectileColor != ColorComponent->GetCurrentColor()) {
		return false;
	}

	CurrentHealth = FMath::Max(CurrentHealth - 1, 0);

	if (CurrentHealth > 0) {
		return true;
	}

	bEliminated = true;
	SetActionState(ERGBEnemyActionState::Disabled);
	ReleaseCoordinatedAttackPermission();

	GetCharacterMovement()->StopMovementImmediately();
	ConsumeMovementInputVector();

	TransferColorSupportingPlatform();
	OnEnemyEliminated.Broadcast(this);

	if (!bArenaRespawnManaged) {
		Destroy();
	}
	return true;
}

void ARGBEnemyCharacter::DespawnForArenaCompletion()
{
	if (IsActorBeingDestroyed()) {
		return;
	}

	bArenaRespawnManaged = false;
	bEliminated = true;
	SetActionState(ERGBEnemyActionState::Disabled);
	ReleaseCoordinatedAttackPermission();

	GetCharacterMovement()->StopMovementImmediately();
	ConsumeMovementInputVector();

	Destroy();
}

void ARGBEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = FMath::Max(MaxHealth, 1);
	bEliminated = false;
	BaseGroundMovementSpeed = FMath::Max(GetCharacterMovement()->MaxWalkSpeed, 0.0f);
	bHasCapturedGroundMovementSpeed = true;
	HandleArenaMovementSpeedMultiplierChanged(ArenaMovementSpeedMultiplier);
	SetActionState(bPatrolEnabled ? ERGBEnemyActionState::Patrol : ERGBEnemyActionState::Idle);

	if (IsValid(ColorComponent)) {
		ApplyColorMaterial(ColorComponent->GetCurrentColor());
	}
	else {
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s has no valid ColorComponent."),
			*GetName());
	}

	if (bPatrolEnabled) {
		PatrolDirection = GetActorForwardVector().X >= 0.0f ? 1.0f : -1.0f;

		SetActorRotation(FRotator(0.0f, PatrolDirection > 0.0f ? 0.0f : 180.0f, 0.0f));

		GetWorldTimerManager().SetTimer(DetectionTimer, this, &ARGBEnemyCharacter::UpdatePlayerDetection, 0.1f, true);
	}
}

void ARGBEnemyCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DetectionTimer);
	GetWorldTimerManager().ClearTimer(AttackDamageTimer);
	GetWorldTimerManager().ClearTimer(AttackFinishTimer);
	ReleaseCoordinatedAttackPermission();
	EnemyCoordinator = nullptr;
	TargetPlayer.Reset();
	Super::EndPlay(EndPlayReason);
}

bool ARGBEnemyCharacter::IsPlayerWithinAttackRange(const ACharacter* Player) const
{
	if (!IsValid(Player)) {
		return false;
	}

	const float DistanceX = FMath::Abs(Player->GetActorLocation().X - GetActorLocation().X);
	const float AllowedDistance = GetCapsuleComponent()->GetScaledCapsuleRadius() + Player->GetCapsuleComponent()->GetScaledCapsuleRadius() + FMath::Max(StopDistanceFromPlayer, 0.0f) + FMath::Max(AttackHitRangePadding, 0.0f);
	const float EnemyFeetZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float PlayerFeetZ = Player->GetActorLocation().Z - Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	return DistanceX <= AllowedDistance && FMath::Abs(PlayerFeetZ - EnemyFeetZ) <= FMath::Max(DetectionHeightTolerance, 0.0f);
}

void ARGBEnemyCharacter::StartAttackAgainst(ACharacter* Player)
{
	if (!IsPlayerWithinAttackRange(Player)) {
		return;
	}

	TargetPlayer = Player;
	StartAttack();
}

void ARGBEnemyCharacter::SetEnemyCoordinator(URGBArenaEnemyCoordinatorComponent* NewCoordinator)
{
	if (EnemyCoordinator == NewCoordinator) {
		return;
	}

	ReleaseCoordinatedAttackPermission();
	EnemyCoordinator = NewCoordinator;
}

void ARGBEnemyCharacter::SetArenaCameraZone(ARGBArenaCameraZone* NewArenaCameraZone)
{
	ArenaCameraZone = NewArenaCameraZone;
}

void ARGBEnemyCharacter::SetOwningArena(ARGBArenaControl* NewArena)
{
	if (OwningArena && NewArena && OwningArena != NewArena) {
		UE_LOG(LogTemp, Warning, TEXT("%s already belongs to %s; rejected arena %s."), *GetName(), *GetNameSafe(OwningArena), *GetNameSafe(NewArena));
		return;
	}
	OwningArena = NewArena;
}

void ARGBEnemyCharacter::ApplyArenaAttributes(const FRGBEnemyArenaAttributes& NewAttributes)
{
	const bool bChanged = !FMath::IsNearlyEqual(ArenaAttributes.Completion, NewAttributes.Completion) || ArenaAttributes.bEnabled != NewAttributes.bEnabled;
	ArenaAttributes = NewAttributes;
	if (bChanged) {
		HandleArenaAttributesChanged();
	}
}

void ARGBEnemyCharacter::HandleArenaAttributesChanged()
{
	HandleArenaMovementSpeedMultiplierChanged(ArenaMovementSpeedMultiplier);
}

bool ARGBEnemyCharacter::CanStartAttackNow() const
{
	return IsAvailableForArenaAI() && !bAttacking && !bTurning && GetWorld() && GetWorld()->GetTimeSeconds() >= NextAttackAllowedTime;
}

bool ARGBEnemyCharacter::IsPlayerVisibleForChase(const ACharacter* Player) const
{
	if (!IsValid(Player) || !IsValid(Player->GetCapsuleComponent())) {
		return false;
	}
	const float Range = ArenaAttributes.bEnabled ? ArenaAttributes.DetectionRange : FMath::Max(DetectionRange, LoseInterestRange);
	FVector Difference = Player->GetActorLocation() - GetActorLocation();
	Difference.Y = 0.0f;
	const float EnemyFeet = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float PlayerFeet = Player->GetActorLocation().Z - Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	if (Difference.SizeSquared() > FMath::Square(FMath::Max(0.0f, Range)) || FMath::Abs(EnemyFeet - PlayerFeet) > FMath::Max(0.0f, DetectionHeightTolerance)) {
		return false;
	}
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(Player);
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Player->GetActorLocation(), ECC_Visibility, Params);
}

bool ARGBEnemyCharacter::CanBeAttackCandidate(const ACharacter* Player) const
{
	if (!IsAvailableForArenaAI() || !IsPlayerVisibleForChase(Player) || !GetCharacterMovement()->IsMovingOnGround()) {
		return false;
	}
	const float Direction = Player->GetActorLocation().X >= GetActorLocation().X ? 1.0f : -1.0f;
	return IsPlayerWithinAttackRange(Player) || CanWalkInDirection(Direction, 0.0f);
}

bool ARGBEnemyCharacter::IsReadyToAttack(const ACharacter* Player) const
{
	return CanStartAttackNow() && IsPlayerWithinAttackRange(Player);
}

void ARGBEnemyCharacter::SetArenaAIActive(bool bNewActive)
{
	if (bArenaAIActive == bNewActive) {
		return;
	}

	bArenaAIActive = bNewActive;

	if (!bArenaAIActive) {
		GetWorldTimerManager().ClearTimer(AttackDamageTimer);
		GetWorldTimerManager().ClearTimer(AttackFinishTimer);
		bAttacking = false;
		bTurning = false;
		TargetPlayer.Reset();
		ConsumeMovementInputVector();
		GetCharacterMovement()->StopMovementImmediately();
		ReleaseCoordinatedAttackPermission();
		SetActionState(ERGBEnemyActionState::Idle);

		if (UAnimInstance* Animation = GetMesh()->GetAnimInstance()) {
			if (AttackMontage) {
				Animation->Montage_Stop(0.1f, AttackMontage.Get());
			}
			if (TurnMontage) {
				Animation->Montage_Stop(0.1f, TurnMontage.Get());
			}
		}
	}

	HandleArenaAIActiveChanged(bArenaAIActive);
}

void ARGBEnemyCharacter::SetArenaRespawnManaged(bool bManaged)
{
	bArenaRespawnManaged = bManaged;
}

void ARGBEnemyCharacter::PrepareForArenaRespawn()
{
	const bool bWasArenaAIActive = bArenaAIActive;
	SetArenaAIActive(false);

	if (!bWasArenaAIActive) {
		HandleArenaAIActiveChanged(false);
	}

	GetWorldTimerManager().ClearTimer(AttackDamageTimer);
	GetWorldTimerManager().ClearTimer(AttackFinishTimer);
	bAttacking = false;
	bTurning = false;
	TargetPlayer.Reset();
	ReleaseCoordinatedAttackPermission();
	ConsumeMovementInputVector();

	if (UCharacterMovementComponent* Movement = GetCharacterMovement()) {
		Movement->StopMovementImmediately();
	}

	if (UAnimInstance* Animation = GetMesh()->GetAnimInstance()) {
		Animation->StopAllMontages(0.0f);
	}

	SetActionState(ERGBEnemyActionState::Disabled);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void ARGBEnemyCharacter::RestoreForArenaRespawn(const FTransform& SpawnTransform)
{
	SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);

	CurrentHealth = FMath::Max(MaxHealth, 1);
	bEliminated = false;
	bAttacking = false;
	bTurning = false;
	NextAttackAllowedTime = 0.0;
	NextPatrolDirectionTime = 0.0;
	TargetPlayer.Reset();

	if (UCharacterMovementComponent* Movement = GetCharacterMovement()) {
		Movement->StopMovementImmediately();
	}

	if (IsValid(ColorComponent)) {
		ColorComponent->ResetColor();
		ApplyColorMaterial(ColorComponent->GetCurrentColor());
	}

	SetActionState(bPatrolEnabled ? ERGBEnemyActionState::Patrol : ERGBEnemyActionState::Idle);
	HandleArenaMovementSpeedMultiplierChanged(ArenaMovementSpeedMultiplier);
}

void ARGBEnemyCharacter::SetArenaMovementSpeedMultiplier(float NewMultiplier)
{
	const float SafeMultiplier = FMath::Max(NewMultiplier, 0.0f);

	if (FMath::IsNearlyEqual(ArenaMovementSpeedMultiplier, SafeMultiplier)) {
		return;
	}

	ArenaMovementSpeedMultiplier = SafeMultiplier;
	HandleArenaMovementSpeedMultiplierChanged(ArenaMovementSpeedMultiplier);
}

void ARGBEnemyCharacter::HandleArenaAIActiveChanged(bool bNewActive)
{
}

void ARGBEnemyCharacter::HandleArenaMovementSpeedMultiplierChanged(float NewMultiplier)
{
	if (!bHasCapturedGroundMovementSpeed) {
		return;
	}
	if (UCharacterMovementComponent* Movement = GetCharacterMovement()) {
		Movement->MaxWalkSpeed = BaseGroundMovementSpeed * FMath::Max(NewMultiplier, 0.0f);
	}
}

void ARGBEnemyCharacter::UpdateGroundMovementSpeed(bool bChasing)
{
	if (ArenaAttributes.bEnabled) {
		const float Multiplier = bChasing ? ArenaAttributes.ChaseSpeedMultiplier : ArenaAttributes.MovementSpeedMultiplier;
		GetCharacterMovement()->MaxWalkSpeed = BaseGroundMovementSpeed * FMath::Max(0.0f, Multiplier);
	}
}

bool ARGBEnemyCharacter::GetCoordinationAssignment(FRGBEnemyCoordinationAssignment& OutAssignment) const
{
	return IsValid(EnemyCoordinator) && EnemyCoordinator->GetAssignment(this, OutAssignment);
}

void ARGBEnemyCharacter::SetActionState(ERGBEnemyActionState NewState)
{
	ActionState = NewState;
}

bool ARGBEnemyCharacter::TryAcquireCoordinatedAttackPermission()
{
	return !IsValid(EnemyCoordinator) || EnemyCoordinator->TryAcquireAttackPermission(this);
}

void ARGBEnemyCharacter::ReleaseCoordinatedAttackPermission()
{
	if (IsValid(EnemyCoordinator)) {
		EnemyCoordinator->ReleaseAttackPermission(this);
	}
}

bool ARGBEnemyCharacter::IsCoordinationTargetReservedByOther(const AActor* Target) const
{
	return IsValid(EnemyCoordinator) && EnemyCoordinator->IsTargetReservedByOther(this, Target);
}

bool ARGBEnemyCharacter::TryReserveCoordinationTarget(AActor* Target)
{
	return !IsValid(EnemyCoordinator) || EnemyCoordinator->TryReserveTarget(this, Target);
}

void ARGBEnemyCharacter::ReleaseCoordinationTarget(const AActor* Target)
{
	if (IsValid(EnemyCoordinator)) {
		EnemyCoordinator->ReleaseTargetReservation(this, Target);
	}
}

bool ARGBEnemyCharacter::UpdateCoordinatedMovement(float DeltaTime)
{
	FRGBEnemyCoordinationAssignment Assignment;

	if (!GetCoordinationAssignment(Assignment) || !Assignment.bHasTargetLocation) {
		return false;
	}
	if (ArenaAttributes.bEnabled && !IsPlayerVisibleForChase(UGameplayStatics::GetPlayerCharacter(this, 0))) {
		return false;
	}
	UpdateGroundMovementSpeed(true);
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (Assignment.bCanAttack && IsReadyToAttack(Player)) {
		StartAttackAgainst(Player);
		if (bAttacking) {
			return true;
		}
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	const float DifferenceX = Assignment.TargetLocation.X - GetActorLocation().X;
	const float SlotTolerance = 10.0f;

	if (FMath::Abs(DifferenceX) <= SlotTolerance) {
		Movement->StopMovementImmediately();
		ConsumeMovementInputVector();
		const bool bAttackCooldownActive = GetWorld()->GetTimeSeconds() < NextAttackAllowedTime;
		SetActionState(bAttackCooldownActive ? ERGBEnemyActionState::AttackCooldown : Assignment.Role == ERGBEnemyCoordinationRole::MeleeAttacker ? ERGBEnemyActionState::Chase : ERGBEnemyActionState::Idle);

		if (Assignment.bCanAttack) {
			StartAttackAgainst(UGameplayStatics::GetPlayerCharacter(this, 0));
		}

		return true;
	}

	const float DesiredDirection = DifferenceX > 0.0f ? 1.0f : -1.0f;

	if (DesiredDirection != PatrolDirection) {
		StartTurn(DesiredDirection);
		return true;
	}

	SetActionState(ERGBEnemyActionState::Chase);

	if (!CanWalkInDirection(PatrolDirection, DeltaTime)) {
		Movement->StopMovementImmediately();
		return true;
	}

	AddMovementInput(FVector::ForwardVector, PatrolDirection);
	return true;
}

bool ARGBEnemyCharacter::CanWalkInDirection(float Direction, float DeltaTime) const
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	const UCharacterMovementComponent* Movement = GetCharacterMovement();

	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();

	const float TravelDistance = Movement->MaxWalkSpeed * FMath::Max(DeltaTime, 0.0f);
	const float ProbeDistance = Radius + FMath::Max(EdgeLookAhead, 0.0f) + TravelDistance;

	const FVector Forward(Direction, 0.0f, 0.0f);
	const FVector Location = GetActorLocation();

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FHitResult WallHit;
	const bool bWallAhead = GetWorld()->SweepSingleByChannel(WallHit, Location, Location + Forward * ProbeDistance, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(Radius * 0.8f), QueryParams);

	if (bWallAhead)
	{
		return false;
	}

	const FVector Feet = Location - FVector(0.0f, 0.0f, HalfHeight);
	const FVector GroundAhead = Feet + Forward * ProbeDistance;
	const FVector TraceStart = GroundAhead + FVector(0.0f, 0.0f, Movement->MaxStepHeight + 5.0f);
	const FVector TraceEnd = GroundAhead - FVector(0.0f, 0.0f, FMath::Max(MaxPatrolDrop, 0.0f) + 5.0f);

	FHitResult GroundHit;
	const bool bGroundAhead = GetWorld()->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Pawn, QueryParams);

	const bool bSafeGround = bGroundAhead && Movement->IsWalkable(GroundHit);

	return bSafeGround;
}

void ARGBEnemyCharacter::UpdatePlayerDetection()
{
	ACharacter* Player = UGameplayStatics::GetPlayerCharacter(this, 0);

	if (!bArenaAIActive || !bPatrolEnabled || !IsValid(Player) || Player == this) {
		TargetPlayer.Reset();
		return;
	}
	if (ArenaAttributes.bEnabled) {
		TargetPlayer = IsPlayerVisibleForChase(Player) ? Player : nullptr;
		return;
	}

	const FVector EnemyLocation = GetActorLocation();
	const FVector PlayerLocation = Player->GetActorLocation();

	const float EnemyFeetZ = EnemyLocation.Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float PlayerFeetZ = PlayerLocation.Z - Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float AllowedRange = TargetPlayer.Get() == Player ? FMath::Max(DetectionRange, LoseInterestRange) : DetectionRange;

	const bool bWithinRange = FVector::DistSquared2D(EnemyLocation, PlayerLocation) <= FMath::Square(FMath::Max(AllowedRange, 0.0f));
	const bool bSimilarHeight = FMath::Abs(PlayerFeetZ - EnemyFeetZ) <= FMath::Max(DetectionHeightTolerance, 0.0f);

	if (!bWithinRange || !bSimilarHeight) {
		TargetPlayer.Reset();
		return;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(Player);

	FHitResult SightHit;
	const bool bSightBlocked = GetWorld()->LineTraceSingleByChannel(SightHit, EnemyLocation, PlayerLocation, ECC_Visibility, QueryParams);

	if (bSightBlocked) {
		TargetPlayer.Reset();
		return;
	}

	TargetPlayer = Player;
}

void ARGBEnemyCharacter::StartTurn(float NewDirection)
{
	if (bTurning || bAttacking ||NewDirection == PatrolDirection) {
		return;
	}

	GetCharacterMovement()->StopMovementImmediately();
	ConsumeMovementInputVector();

	TurnStartYaw = GetActorRotation().Yaw;
	PatrolDirection = NewDirection;

	UAnimInstance* Animation = GetMesh()->GetAnimInstance();

	if (!Animation || !TurnMontage
		|| TurnMontage->GetPlayLength() <= SMALL_NUMBER) {
		UE_LOG(LogTemp, Warning,
			TEXT("%s: Turn requires an Animation Blueprint and TurnMontage."),
			*GetName());

		SetActorRotation(FRotator(
			0.0f, PatrolDirection > 0.0f ? 0.0f : 180.0f, 0.0f));
		return;
	}

	if (Animation->Montage_Play(TurnMontage.Get(), 1.0f) <= 0.0f) {
		UE_LOG(LogTemp, Warning,
			TEXT("%s: Could not play TurnMontage."),
			*GetName());

		SetActorRotation(FRotator(
			0.0f, PatrolDirection > 0.0f ? 0.0f : 180.0f, 0.0f));
		return;
	}

	TurnDuration = TurnMontage->GetPlayLength();
	bTurning = true;
	SetActionState(ERGBEnemyActionState::Turning);
}

void ARGBEnemyCharacter::UpdateTurn(float DeltaTime)
{
	UAnimInstance* Animation = GetMesh()->GetAnimInstance();

	if (!Animation || !TurnMontage
		|| !Animation->Montage_IsActive(TurnMontage.Get())) {
		bTurning = false;

		SetActorRotation(FRotator(
			0.0f, PatrolDirection > 0.0f ? 0.0f : 180.0f, 0.0f));
		return;
	}

	const float Position =
		Animation->Montage_GetPosition(TurnMontage.Get());

	const float Alpha = FMath::Clamp(
		Position / FMath::Max(TurnDuration, SMALL_NUMBER),
		0.0f, 1.0f);

	SetActorRotation(FRotator(
		0.0f, TurnStartYaw - 180.0f * Alpha, 0.0f));
}

void ARGBEnemyCharacter::StartAttack()
{
	if (!CanStartAttackNow()) {
		return;
	}

	if (!TryAcquireCoordinatedAttackPermission()) {
		return;
	}

	GetCharacterMovement()->StopMovementImmediately();
	ConsumeMovementInputVector();

	const float SafePlayRate = FMath::Max(AttackPlayRate, 0.01f);
	UAnimInstance* Animation = GetMesh()->GetAnimInstance();

	if (Animation && AttackMontage) {
		Animation->Montage_Play(AttackMontage.Get(), SafePlayRate);
	}

	bAttacking = true;
	SetActionState(ERGBEnemyActionState::Attacking);
	CurrentAttackCooldown = FMath::Max(AttackCooldown, 0.0f) * (ArenaAttributes.bEnabled ? ArenaAttributes.AttackCooldownMultiplier : 1.0f);

	GetWorldTimerManager().ClearTimer(AttackDamageTimer);
	GetWorldTimerManager().ClearTimer(AttackFinishTimer);

	const float DamageDelay = FMath::Max(AttackHitDelay, 0.0f);
	const float SafeAttackDuration = FMath::Max(AttackDuration, DamageDelay + 0.01f);

	if (DamageDelay <= SMALL_NUMBER) {
		TryApplyAttackDamage();
	}
	else {
		GetWorldTimerManager().SetTimer(AttackDamageTimer, this, &ARGBEnemyCharacter::TryApplyAttackDamage, DamageDelay, false);
	}

	GetWorldTimerManager().SetTimer(AttackFinishTimer, this, &ARGBEnemyCharacter::FinishAttack, SafeAttackDuration, false);
}

void ARGBEnemyCharacter::FinishAttack()
{
	bAttacking = false;
	NextAttackAllowedTime = GetWorld()->GetTimeSeconds() + CurrentAttackCooldown;
	SetActionState(ERGBEnemyActionState::AttackCooldown);
	ReleaseCoordinatedAttackPermission();
}

void ARGBEnemyCharacter::TryApplyAttackDamage()
{
	if (!bAttacking || !IsAvailableForArenaAI()) {
		return;
	}
	ARGBPlayerCharacter* Player =
		Cast<ARGBPlayerCharacter>(TargetPlayer.Get());

	if (!IsValid(Player) || AttackDamage <= 0 || !IsPlayerWithinAttackRange(Player)) {
		return;
	}

	Player->RecieveDamageFromSource(AttackDamage, this);
}

void ARGBEnemyCharacter::ApplyColorMaterial(ERGBColor Color)
{
	UMaterialInterface* BaseMaterial = nullptr;
	UMaterialInterface* EmissionMaterial = nullptr;

	switch (Color) {
	case ERGBColor::Red:
		BaseMaterial = RedBaseMaterial.Get();
		EmissionMaterial = RedEmissionMaterial.Get();
		break;

	case ERGBColor::Green:
		BaseMaterial = GreenBaseMaterial.Get();
		EmissionMaterial = GreenEmissionMaterial.Get();
		break;

	case ERGBColor::Blue:
		BaseMaterial = BlueBaseMaterial.Get();
		EmissionMaterial = BlueEmissionMaterial.Get();
		break;

	default:
		ensureMsgf(false, TEXT("%s has an invalid enemy color"), *GetName());
	}

	if (!IsValid(GetMesh())) {
		return;
	}

	if (IsValid(BaseMaterial)) {
		GetMesh()->SetMaterial(0, BaseMaterial);
	}
	else {
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s is missing its base material for color %d."),
			*GetName(),
			static_cast<int32>(Color));
	}

	if (IsValid(EmissionMaterial)) {
		GetMesh()->SetMaterial(1, EmissionMaterial);
	}
	else {
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s is missing its emission material for color %d."),
			*GetName(),
			static_cast<int32>(Color));
	}
}

void ARGBEnemyCharacter::TransferColorSupportingPlatform()
{
	if (!IsValid(ColorComponent)) {
		return;
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (!IsValid(Movement) || !Movement->IsMovingOnGround()) {
		return;
	}

	ARGBColorPlatform* SupportingPlatform = Cast<ARGBColorPlatform>(Movement->CurrentFloor.HitResult.GetActor());

	if (!IsValid(SupportingPlatform)) {
		return;
	}

	URGBColorComponent* PlatformColor = SupportingPlatform->GetColorComponent();

	if (!IsValid(PlatformColor)) {
		return;
	}

	PlatformColor->SetColor(ColorComponent->GetCurrentColor());
}

