#include "RGBArenaEnemyCoordinatorComponent.h"

#include "RGBEnemyCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogRGBAI, Log, All);

URGBArenaEnemyCoordinatorComponent::URGBArenaEnemyCoordinatorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URGBArenaEnemyCoordinatorComponent::BeginPlay()
{
	Super::BeginPlay();
	const float SafeInterval = FMath::Max(AssignmentUpdateInterval, 0.05f);
	GetWorld()->GetTimerManager().SetTimer(AssignmentTimer, this, &URGBArenaEnemyCoordinatorComponent::UpdateAssignments, SafeInterval, true, 0.0f);
}

void URGBArenaEnemyCoordinatorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(AssignmentTimer);
	}
	ResetCoordination();
	Super::EndPlay(EndPlayReason);
}

void URGBArenaEnemyCoordinatorComponent::RegisterEnemy(ARGBEnemyCharacter* Enemy, ERGBEnemyMobilityType MobilityType)
{
	if (!IsValid(Enemy)) {
		return;
	}

	if (FRegisteredEnemy* ExistingRecord = FindRegisteredEnemy(Enemy)) {
		ExistingRecord->MobilityType = MobilityType;
		return;
	}

	FRegisteredEnemy& NewRecord = RegisteredEnemies.AddDefaulted_GetRef();
	NewRecord.Enemy = Enemy;
	NewRecord.MobilityType = MobilityType;
	RefreshDebugCounters();
	UpdateAssignments();
}

void URGBArenaEnemyCoordinatorComponent::UnregisterEnemy(ARGBEnemyCharacter* Enemy)
{
	ReleaseTargetReservation(Enemy);
	RegisteredEnemies.RemoveAll([Enemy](const FRegisteredEnemy& Record) { return !Record.Enemy.IsValid() || Record.Enemy.Get() == Enemy; });
	RefreshDebugCounters();
	UpdateAssignments();
}

void URGBArenaEnemyCoordinatorComponent::ResetCoordination()
{
	RegisteredEnemies.Reset();
	TargetReservations.Reset();
	PlayerTarget.Reset();
	RefreshDebugCounters();
}

void URGBArenaEnemyCoordinatorComponent::SetPlayerTarget(ACharacter* NewPlayerTarget)
{
	PlayerTarget = NewPlayerTarget;
	UpdateAssignments();
}

void URGBArenaEnemyCoordinatorComponent::ConfigureArenaPolicy(float DecisionInterval, float RoleCommitment, int32 MaximumAttackers)
{
	bUseArenaPolicy = true;
	AssignmentUpdateInterval = FMath::Max(0.05f, DecisionInterval);
	MinimumAssignmentCommitment = FMath::Max(0.0f, RoleCommitment);
	MaximumSimultaneousAttackers = FMath::Max(0, MaximumAttackers);
	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().SetTimer(AssignmentTimer, this, &URGBArenaEnemyCoordinatorComponent::UpdateAssignments, AssignmentUpdateInterval, true);
	}
	UpdateAssignments();
}

void URGBArenaEnemyCoordinatorComponent::SetMaximumAttackers(int32 MaximumAttackers)
{
	const int32 NewMaximum = FMath::Max(0, MaximumAttackers);
	if (MaximumSimultaneousAttackers != NewMaximum) {
		MaximumSimultaneousAttackers = NewMaximum;
		UpdateAssignments();
	}
}

bool URGBArenaEnemyCoordinatorComponent::GetAssignment(const ARGBEnemyCharacter* Enemy, FRGBEnemyCoordinationAssignment& OutAssignment) const
{
	const FRegisteredEnemy* Record = FindRegisteredEnemy(Enemy);
	const UWorld* World = GetWorld();

	if (!Record || !World || Record->Assignment.LeaseExpiresAt < World->GetTimeSeconds()) {
		return false;
	}

	OutAssignment = Record->Assignment;
	return true;
}

bool URGBArenaEnemyCoordinatorComponent::CanEnemyAttack(const ARGBEnemyCharacter* Enemy) const
{
	FRGBEnemyCoordinationAssignment Assignment;
	return GetAssignment(Enemy, Assignment) && Assignment.bCanAttack;
}

bool URGBArenaEnemyCoordinatorComponent::TryAcquireAttackPermission(ARGBEnemyCharacter* Enemy)
{
	FRegisteredEnemy* Record = FindRegisteredEnemy(Enemy);

	if (!Record || !CanEnemyAttack(Enemy)) {
		return false;
	}

	int32 AcquiredPermissionCount = 0;
	for (const FRegisteredEnemy& OtherRecord : RegisteredEnemies) {
		AcquiredPermissionCount += OtherRecord.bAttackPermissionAcquired ? 1 : 0;
	}

	if (!Enemy->IsAvailableForArenaAI() || (!Record->bAttackPermissionAcquired && AcquiredPermissionCount >= FMath::Max(MaximumSimultaneousAttackers, 0))) {
		return false;
	}

	Record->bAttackPermissionAcquired = true;
	RefreshDebugCounters();
	return true;
}

void URGBArenaEnemyCoordinatorComponent::ReleaseAttackPermission(ARGBEnemyCharacter* Enemy)
{
	if (FRegisteredEnemy* Record = FindRegisteredEnemy(Enemy)) {
		Record->bAttackPermissionAcquired = false;
	}
	RefreshDebugCounters();
}

bool URGBArenaEnemyCoordinatorComponent::IsTargetReservedByOther(const ARGBEnemyCharacter* Enemy, const AActor* Target) const
{
	if (!IsValid(Target)) {
		return true;
	}

	for (const FTargetReservation& Reservation : TargetReservations) {
		if (Reservation.Target.Get() == Target && Reservation.Enemy.IsValid() && Reservation.Enemy.Get() != Enemy) {
			return true;
		}
	}

	return false;
}

bool URGBArenaEnemyCoordinatorComponent::TryReserveTarget(ARGBEnemyCharacter* Enemy, AActor* Target)
{
	if (!IsValid(Enemy) || !IsValid(Target) || IsTargetReservedByOther(Enemy, Target)) {
		return false;
	}

	ReleaseTargetReservation(Enemy);
	FTargetReservation& Reservation = TargetReservations.AddDefaulted_GetRef();
	Reservation.Target = Target;
	Reservation.Enemy = Enemy;
	RefreshDebugCounters();
	return true;
}

void URGBArenaEnemyCoordinatorComponent::ReleaseTargetReservation(ARGBEnemyCharacter* Enemy, const AActor* Target)
{
	TargetReservations.RemoveAll([Enemy, Target](const FTargetReservation& Reservation) { return !Reservation.Enemy.IsValid() || !Reservation.Target.IsValid() || (Reservation.Enemy.Get() == Enemy && (!Target || Reservation.Target.Get() == Target)); });
	RefreshDebugCounters();
}

void URGBArenaEnemyCoordinatorComponent::UpdateAssignments()
{
	RemoveInvalidEnemies();

	ACharacter* Player = PlayerTarget.Get();
	if (!IsValid(Player)) {
		Player = UGameplayStatics::GetPlayerCharacter(this, 0);
		PlayerTarget = Player;
	}

	UWorld* World = GetWorld();
	if (!IsValid(Player) || !World) {
		return;
	}

	const FVector PlayerLocation = Player->GetActorLocation();
	const double CurrentTime = World->GetTimeSeconds();
	TArray<FRegisteredEnemy*> AttackCandidates;

	for (FRegisteredEnemy& Record : RegisteredEnemies) {
		ARGBEnemyCharacter* Enemy = Record.Enemy.Get();
		if (bUseArenaPolicy) {
			if (CurrentTime >= Record.ChaseDecisionExpiresAt) {
				Record.bWantsChase = FMath::FRand() < Enemy->GetChasePreference();
				Record.ChaseDecisionExpiresAt = CurrentTime + FMath::Max(MinimumAssignmentCommitment, AssignmentUpdateInterval);
			}
			const bool bGroundWantsChase = Record.MobilityType != ERGBEnemyMobilityType::Ground || Record.bWantsChase || Enemy->IsReadyToAttack(Player);
			if (Record.bAttackPermissionAcquired || (bGroundWantsChase && Enemy->CanBeAttackCandidate(Player))) {
				AttackCandidates.Add(&Record);
			}
		}
		else if (Record.MobilityType != ERGBEnemyMobilityType::Teleporter) {
			AttackCandidates.Add(&Record);
		}
	}

	AttackCandidates.Sort([this, Player, PlayerLocation, CurrentTime](const FRegisteredEnemy& Left, const FRegisteredEnemy& Right) {
		const ARGBEnemyCharacter* LeftEnemy = Left.Enemy.Get();
		const ARGBEnemyCharacter* RightEnemy = Right.Enemy.Get();
		if (bUseArenaPolicy) {
			if (Left.bAttackPermissionAcquired != Right.bAttackPermissionAcquired) {
				return Left.bAttackPermissionAcquired;
			}
			const bool bLeftReady = LeftEnemy->IsReadyToAttack(Player);
			const bool bRightReady = RightEnemy->IsReadyToAttack(Player);
			if (bLeftReady != bRightReady) {
				return bLeftReady;
			}
		}
		const bool bLeftCommittedAttacker = Left.Assignment.Role == ERGBEnemyCoordinationRole::MeleeAttacker && (Left.bAttackPermissionAcquired || Left.CommitmentExpiresAt > CurrentTime);
		const bool bRightCommittedAttacker = Right.Assignment.Role == ERGBEnemyCoordinationRole::MeleeAttacker && (Right.bAttackPermissionAcquired || Right.CommitmentExpiresAt > CurrentTime);
		if (bLeftCommittedAttacker != bRightCommittedAttacker) {
			return bLeftCommittedAttacker;
		}
		FVector LeftDelta = LeftEnemy->GetActorLocation() - PlayerLocation;
		FVector RightDelta = RightEnemy->GetActorLocation() - PlayerLocation;
		LeftDelta.Y = RightDelta.Y = 0.0f;
		if (bUseArenaPolicy && Left.MobilityType == ERGBEnemyMobilityType::Teleporter) { LeftDelta.Z = 0.0f; }
		if (bUseArenaPolicy && Right.MobilityType == ERGBEnemyMobilityType::Teleporter) { RightDelta.Z = 0.0f; }
		return LeftDelta.SizeSquared() < RightDelta.SizeSquared();
	});

	int32 AttackerCount = FMath::Min(FMath::Max(MaximumSimultaneousAttackers, bUseArenaPolicy ? 0 : 1), AttackCandidates.Num());
	// A falling budget never cancels an attack after its animation has started.
	if (bUseArenaPolicy) {
		int32 InFlightCount = 0;
		for (const FRegisteredEnemy* Candidate : AttackCandidates) { InFlightCount += Candidate->bAttackPermissionAcquired ? 1 : 0; }
		AttackerCount = FMath::Max(AttackerCount, InFlightCount);
	}
	TSet<const ARGBEnemyCharacter*> SelectedAttackers;
	for (int32 Index = 0; Index < AttackerCount; ++Index) {
		SelectedAttackers.Add(AttackCandidates[Index]->Enemy.Get());
	}

	int32 PressureIndex = 0;
	for (FRegisteredEnemy& Record : RegisteredEnemies) {
		ARGBEnemyCharacter* Enemy = Record.Enemy.Get();
		const bool bSelectedAttacker = SelectedAttackers.Contains(Enemy);
		const bool bCommitmentActive = Record.CommitmentExpiresAt > CurrentTime;
		ERGBEnemyCoordinationRole DesiredRole = ERGBEnemyCoordinationRole::Pressure;
		FVector DesiredLocation = FVector::ZeroVector;
		int32 DesiredSlotIndex = INDEX_NONE;
		bool bDesiredAttackPermission = false;
		bool bDesiredMovementTarget = true;

		if (Record.MobilityType == ERGBEnemyMobilityType::Teleporter) {
			DesiredRole = ERGBEnemyCoordinationRole::Special;
			DesiredLocation = PlayerLocation;
			bDesiredAttackPermission = bUseArenaPolicy ? bSelectedAttacker : true;
		}
		else if (bSelectedAttacker) {
			DesiredRole = ERGBEnemyCoordinationRole::MeleeAttacker;
			DesiredLocation = MakeMeleeSlotLocation(Record, PlayerLocation);
			bDesiredAttackPermission = true;
		}
		else {
			DesiredLocation = MakePressureSlotLocation(Record, PlayerLocation, PressureIndex);
			DesiredSlotIndex = PressureIndex;
			++PressureIndex;
		}
		if (bUseArenaPolicy && Record.MobilityType == ERGBEnemyMobilityType::Ground && !bSelectedAttacker) {
			// Unselected ground enemies patrol instead of pursuing hidden player slots.
			bDesiredMovementTarget = false;
		}
		if (bUseArenaPolicy && !Enemy->IsAvailableForArenaAI()) {
			bDesiredAttackPermission = false;
			bDesiredMovementTarget = false;
		}

		if (bCommitmentActive && Record.Assignment.Role == DesiredRole) {
			Record.Assignment.TargetLocation = DesiredLocation;
			Record.Assignment.bHasTargetLocation = true;
			Record.Assignment.bCanAttack = bDesiredAttackPermission;
			Record.Assignment.SlotIndex = DesiredSlotIndex;
			Record.Assignment.LeaseExpiresAt = CurrentTime + FMath::Max(AssignmentLeaseDuration, AssignmentUpdateInterval * 2.0f);
		}
		else {
			ApplyAssignment(Record, DesiredRole, DesiredLocation, DesiredSlotIndex, bDesiredAttackPermission, CurrentTime);
		}
		Record.Assignment.bHasTargetLocation = bDesiredMovementTarget;
	}
	RefreshDebugCounters();
}

void URGBArenaEnemyCoordinatorComponent::RemoveInvalidEnemies()
{
	RegisteredEnemies.RemoveAll([](const FRegisteredEnemy& Record) { return !Record.Enemy.IsValid(); });
	TargetReservations.RemoveAll([](const FTargetReservation& Reservation) { return !Reservation.Enemy.IsValid() || !Reservation.Target.IsValid(); });
	RefreshDebugCounters();
}

URGBArenaEnemyCoordinatorComponent::FRegisteredEnemy* URGBArenaEnemyCoordinatorComponent::FindRegisteredEnemy(const ARGBEnemyCharacter* Enemy)
{
	return RegisteredEnemies.FindByPredicate([Enemy](const FRegisteredEnemy& Record) { return Record.Enemy.Get() == Enemy; });
}

const URGBArenaEnemyCoordinatorComponent::FRegisteredEnemy* URGBArenaEnemyCoordinatorComponent::FindRegisteredEnemy(const ARGBEnemyCharacter* Enemy) const
{
	return RegisteredEnemies.FindByPredicate([Enemy](const FRegisteredEnemy& Record) { return Record.Enemy.Get() == Enemy; });
}

FVector URGBArenaEnemyCoordinatorComponent::MakeMeleeSlotLocation(const FRegisteredEnemy& Record, const FVector& PlayerLocation) const
{
	const ARGBEnemyCharacter* Enemy = Record.Enemy.Get();
	const float Side = Enemy && Enemy->GetActorLocation().X < PlayerLocation.X ? -1.0f : 1.0f;
	FVector SlotLocation = PlayerLocation + FVector(Side * FMath::Max(MeleeSlotDistance, 0.0f), 0.0f, 0.0f);
	if (Enemy && Record.MobilityType == ERGBEnemyMobilityType::Ground) {
		SlotLocation.Z = Enemy->GetActorLocation().Z;
	}
	return SlotLocation;
}

FVector URGBArenaEnemyCoordinatorComponent::MakePressureSlotLocation(const FRegisteredEnemy& Record, const FVector& PlayerLocation, int32 PressureIndex) const
{
	const int32 RingIndex = PressureIndex / 2;
	const float Side = PressureIndex % 2 == 0 ? -1.0f : 1.0f;
	const float HorizontalDistance = FMath::Max(PressureSlotDistance, 0.0f) + RingIndex * FMath::Max(AdditionalPressureSlotSpacing, 0.0f);
	FVector SlotLocation = PlayerLocation + FVector(Side * HorizontalDistance, 0.0f, 0.0f);
	const ARGBEnemyCharacter* Enemy = Record.Enemy.Get();

	if (Record.MobilityType == ERGBEnemyMobilityType::Flying) {
		const float VerticalSide = RingIndex % 2 == 0 ? 1.0f : -1.0f;
		SlotLocation.Z += VerticalSide * (RingIndex + 1) * FMath::Max(FlyingVerticalSlotSpacing, 0.0f);
	}
	else if (Enemy) {
		SlotLocation.Z = Enemy->GetActorLocation().Z;
	}

	return SlotLocation;
}

void URGBArenaEnemyCoordinatorComponent::ApplyAssignment(FRegisteredEnemy& Record, ERGBEnemyCoordinationRole Role, const FVector& TargetLocation, int32 SlotIndex, bool bCanAttack, double CurrentTime)
{
	const ERGBEnemyCoordinationRole PreviousRole = Record.Assignment.Role;

	if (Record.Assignment.Role != Role) {
		// New arena policy preserves an acquired permission until execution finishes.
		if (!bUseArenaPolicy) { Record.bAttackPermissionAcquired = false; }
	}

	Record.Assignment.Role = Role;
	Record.Assignment.TargetLocation = TargetLocation;
	Record.Assignment.bHasTargetLocation = true;
	Record.Assignment.bCanAttack = bCanAttack;
	Record.Assignment.SlotIndex = SlotIndex;
	Record.Assignment.LeaseExpiresAt = CurrentTime + FMath::Max(AssignmentLeaseDuration, AssignmentUpdateInterval * 2.0f);
	Record.CommitmentExpiresAt = CurrentTime + FMath::Max(MinimumAssignmentCommitment, 0.0f);

	if (bEnableDebugLogging && PreviousRole != Role) {
		UE_LOG(LogRGBAI, Log, TEXT("%s assigned %s at slot %d."), *GetNameSafe(Record.Enemy.Get()), *UEnum::GetValueAsString(Role), SlotIndex);
	}
}

void URGBArenaEnemyCoordinatorComponent::RefreshDebugCounters()
{
	RegisteredEnemyCount = RegisteredEnemies.Num();
	ActiveAttackPermissionCount = 0;
	CurrentAssignments.Reset();

	for (const FRegisteredEnemy& Record : RegisteredEnemies) {
		ActiveAttackPermissionCount += Record.bAttackPermissionAcquired ? 1 : 0;
		CurrentAssignments.Add(FString::Printf(TEXT("%s: %s | eligible=%s | executing=%s | movement=%s"), *GetNameSafe(Record.Enemy.Get()), *UEnum::GetValueAsString(Record.Assignment.Role), Record.Assignment.bCanAttack ? TEXT("yes") : TEXT("no"), Record.bAttackPermissionAcquired ? TEXT("yes") : TEXT("no"), Record.Assignment.bHasTargetLocation ? TEXT("assigned") : TEXT("patrol")));
	}

	ActiveTargetReservationCount = TargetReservations.Num();
}
