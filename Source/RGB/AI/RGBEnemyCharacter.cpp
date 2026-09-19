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

#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"


// Sets default values
ARGBEnemyCharacter::ARGBEnemyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

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
		const float DifferenceX = Player->GetActorLocation().X - GetActorLocation().X;

		if (!FMath::IsNearlyZero(DifferenceX)) {
			PatrolDirection = DifferenceX > 0.0f ? 1.0f : -1.0f;
		}

		SetActorRotation(FRotator(0.0f, PatrolDirection > 0.0f ? 0.0f : 180.0f, 0.0f));

		const float RequiredSeparation = GetCapsuleComponent()->GetScaledCapsuleRadius() + Player->GetCapsuleComponent()->GetScaledCapsuleRadius() + FMath::Max(StopDistanceFromPlayer, 0.0f);
		
		if (FMath::Abs(DifferenceX) <= RequiredSeparation || !CanWalkInDirection(PatrolDirection, DeltaTime)) {
			Movement->StopMovementImmediately();
			return;
		}

		AddMovementInput(FVector::ForwardVector, PatrolDirection);
		return;
	}

	if (!CanWalkInDirection(PatrolDirection, DeltaTime)) {
		Movement->StopMovementImmediately();
		PatrolDirection *= -1.0f;

		if (!CanWalkInDirection(PatrolDirection, DeltaTime)) {
			return;
		}
	}

	SetActorRotation(FRotator(0.0f, PatrolDirection > 0.0f ? 0.0f : 180.0f, 0.0f));

	AddMovementInput(FVector::ForwardVector, PatrolDirection);
}

void ARGBEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	PatrolDirection = GetActorForwardVector().X >= 0.0f ? 1.0f : -1.0f;

	SetActorRotation(FRotator(0.0f, PatrolDirection > 0.0f ? 0.0f : 180.0f, 0.0f));
	ResumeWalkAnimation();

	GetWorldTimerManager().SetTimer(DetectionTimer, this, &ARGBEnemyCharacter::UpdatePlayerDetection, 0.1f, true);
}

void ARGBEnemyCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DetectionTimer);
	TargetPlayer.Reset();
	Super::EndPlay(EndPlayReason);
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
	if (bTurning || NewDirection == PatrolDirection) {
		return;
	}

	GetCharacterMovement()->StopMovementImmediately();
	ConsumeMovementInputVector();

	TurnStartYaw = GetActorRotation().Yaw;
	PatrolDirection = NewDirection;

	if (!TurnAnimation || TurnAnimation->GetPlayLength() <= SMALL_NUMBER) {
		SetActorRotation(FRotator(0.0f, PatrolDirection > 0.0f ? 0.0f : 180.0f, 0.0f));
		return;
	}

	bTurning = true;
	TurnElapsed = 0.0f;
	TurnDuration = TurnAnimation->GetPlayLength();

	GetMesh()->PlayAnimation(TurnAnimation.Get(), false);

	if (UAnimSingleNodeInstance* Animation = GetMesh()->GetSingleNodeInstance()){
		Animation->SetPlaying(false);
		Animation->SetPosition(0.0f, false);
	}
}

void ARGBEnemyCharacter::UpdateTurn(float DeltaTime)
{
	TurnElapsed = FMath::Min(TurnElapsed + DeltaTime, TurnDuration);
	const float Alpha = TurnElapsed / TurnDuration;

	SetActorRotation(FRotator(0.0f, TurnStartYaw - 180.0f * Alpha, 0.0f));

	if (UAnimSingleNodeInstance* Animation = GetMesh()->GetSingleNodeInstance()) {
		Animation->SetPosition(TurnElapsed, false);
	}

	if (TurnElapsed >= TurnDuration) {
		bTurning = false;

		SetActorRotation(FRotator(0.0f, PatrolDirection > 0.0f ? 0.0f : 180.0f, 0.0f));
		ResumeWalkAnimation();
	}
}

void ARGBEnemyCharacter::ResumeWalkAnimation()
{
	if (WalkAnimation) {
		GetMesh()->PlayAnimation(WalkAnimation.Get(), true);
	}
}
