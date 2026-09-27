// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBEnemyCharacter.h"

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

void ARGBEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (bAttacking) {
		ConsumeMovementInputVector();

		if (Movement->IsMovingOnGround()) {
			Movement->StopMovementImmediately();
		}

		UpdateAttack();
		return;
	}

	if (bTurning) {
		ConsumeMovementInputVector();

		if (Movement->IsMovingOnGround()) {
			Movement->StopMovementImmediately();
		}

		UpdateTurn(DeltaTime);
		return;
	}

	if (!bPatrolEnabled) {
		if (Movement->IsMovingOnGround()) {
			Movement->StopMovementImmediately();
		}
		return;
	}

	if (!Movement->IsMovingOnGround()) {
		return;
	}

	if (ACharacter* Player = TargetPlayer.Get()) {
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

	GetCharacterMovement()->StopMovementImmediately();
	ConsumeMovementInputVector();

	TransferColorSupportingPlatform();

	Destroy();
	return true;
}

void ARGBEnemyCharacter::DespawnForArenaCompletion()
{
	if (bEliminated) {
		return;
	}

	bEliminated = true;

	GetCharacterMovement()->StopMovementImmediately();
	ConsumeMovementInputVector();

	Destroy();
}

void ARGBEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = FMath::Max(MaxHealth, 1);
	bEliminated = false;

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
		UE_LOG(LogTemp, Warning,
			TEXT("Patrol blocked by wall: %s"),
			*GetNameSafe(WallHit.GetActor()));

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

	if (!bPatrolEnabled || !IsValid(Player) || Player == this) {
		TargetPlayer.Reset();
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
	if (bAttacking || bTurning || GetWorld()->GetTimeSeconds() < NextAttackAllowedTime) {
		return;
	}

	UAnimInstance* Animation = GetMesh()->GetAnimInstance();

	if (!Animation || !AttackMontage) {
		return;
	}

	GetCharacterMovement()->StopMovementImmediately();
	ConsumeMovementInputVector();

	const float SafePlayRate = FMath::Max(AttackPlayRate, 0.01f);

	if (Animation->Montage_Play(AttackMontage.Get(), SafePlayRate) <= 0.0f) {
		NextAttackAllowedTime = GetWorld()->GetTimeSeconds() + 1.0;
		return;
	}

	bAttacking = true;

	GetWorldTimerManager().ClearTimer(AttackDamageTimer);

	const float DamageDelay = FMath::Max(AttackHitDelay, 0.0f) / SafePlayRate;

	if (DamageDelay <= SMALL_NUMBER) {
		TryApplyAttackDamage();
	}
	else {
		GetWorldTimerManager().SetTimer(AttackDamageTimer, this, &ARGBEnemyCharacter::TryApplyAttackDamage, DamageDelay, false);
	}
}

void ARGBEnemyCharacter::UpdateAttack()
{
	UAnimInstance* Animation = GetMesh()->GetAnimInstance();

	if (Animation && AttackMontage && Animation->Montage_IsActive(AttackMontage.Get())) {
		return;
	}

	bAttacking = false;

	NextAttackAllowedTime = GetWorld()->GetTimeSeconds() + FMath::Max(AttackCooldown, 0.0f);
}

void ARGBEnemyCharacter::TryApplyAttackDamage()
{
	ARGBPlayerCharacter* Player =
		Cast<ARGBPlayerCharacter>(TargetPlayer.Get());

	if (!IsValid(Player) || AttackDamage <= 0 || !IsPlayerWithinAttackRange(Player)) {
		return;
	}

	Player->RecieveDamage(AttackDamage);
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

