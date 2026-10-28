// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBArenaControl.h"

#include "Components/TextRenderComponent.h"
#include "Components/SceneComponent.h"
#include "../Camera/RGBArenaCameraZone.h"
#include "../Platforms/RGBColorPlatform.h"

#include "../AI/RGBEnemyCharacter.h"
#include "../AI/RGBArenaEnemyCoordinatorComponent.h"
#include "Engine/World.h"
#include "Engine/TargetPoint.h"

#include "Kismet/GameplayStatics.h"
#include "../Game/RGBDifficultySettings.h"
#include "../Game/RGBGameSettingsSubsystem.h"
#include "Engine/GameInstance.h"

// Sets default values
ARGBArenaControl::ARGBArenaControl()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	EnemyCoordinator = CreateDefaultSubobject<URGBArenaEnemyCoordinatorComponent>(TEXT("EnemyCoordinator"));

	ProgressText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ProgressText"));
	ProgressText->SetupAttachment(SceneRoot);
	ProgressText->SetRelativeLocation(FVector(0.0f, 0.0f, 400.0f));
	ProgressText->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	ProgressText->SetHorizontalAlignment(EHTA_Center);
	ProgressText->SetWorldSize(80.0f);
	ProgressText->SetTextRenderColor(FColor::White);
	ProgressText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProgressText->SetText(FText::FromString(TEXT("Arena Progress")));
}

void ARGBArenaControl::BeginArenaReset()
{
	GetWorldTimerManager().ClearTimer(InitialEvaluationTimer);
	ClearEnemyRespawnTimers();

	if (IsValid(EnemyCoordinator)) {
		EnemyCoordinator->ResetCoordination();
	}

	bResetInProgress = true;
	bCompleted = false;
	MatchingPlatformCount = 0;
	CompletionPercentage = 0.0f;
	ApplyEnemyMovementSpeedMultiplier();

	OnArenaResetStarted.Broadcast();
	OnArenaProgressChanged.Broadcast(0.0f);

	for (ARGBColorPlatform* Platform : AssignedPlatforms) {
		if (IsValid(Platform)) {
			if (URGBColorComponent* Color = Platform->GetColorComponent()) {
				Color->SetColorLocked(false);
			}
		}
	}

	for (ARGBEnemyCharacter* Enemy : AssignedEnemies) {
		if (IsValid(Enemy)) {
			const int32* SpawnRecordIndex = EnemySpawnRecordIndices.Find(Enemy);
			Enemy->OnEnemyEliminated.RemoveDynamic(this, &ARGBArenaControl::HandleEnemyEliminated);
			Enemy->SetEnemyCoordinator(nullptr);
			Enemy->SetArenaCameraZone(nullptr);
			Enemy->SetArenaRespawnManaged(true);
			Enemy->PrepareForArenaRespawn();

			if (SpawnRecordIndex && EnemySpawnRecord.IsValidIndex(*SpawnRecordIndex)) {
				PendingRespawnEnemies.Add(*SpawnRecordIndex, Enemy);
			}
			else {
				Enemy->DespawnForArenaCompletion();
			}
		}
	}

	AssignedEnemies.Reset();
	EnemySpawnRecordIndices.Reset();
	bEnemiesNeedRespawn = true;
}

void ARGBArenaControl::FinishArenaReset()
{
	if (bEnemiesNeedRespawn) {
		RespawnRecordedEnemy();
		bEnemiesNeedRespawn = false;
	}

	bResetInProgress = false;
	UpdateProgress();
}

void ARGBArenaControl::BeginPlay()
{
	ApplyDifficultyPreset();
	Super::BeginPlay();

	if (bUseArenaEnemyTuning) {
		EnemyCoordinator->ConfigureArenaPolicy(CoordinationTuning.DecisionInterval, CoordinationTuning.MinimumRoleCommitment, FMath::Max(0, FMath::FloorToInt(CoordinationTuning.MaximumAttackers.Evaluate(0.0f))));
		for (ATargetPoint* Point : TeleportTargetPoints) {
			if (!IsValid(Point) || (IsValid(ArenaCameraZone) && !ArenaCameraZone->ContainWorldLocation(Point->GetActorLocation()))) {
				UE_LOG(LogTemp, Warning, TEXT("%s has a missing or out-of-zone Teleport Target Point: %s."), *GetName(), *GetNameSafe(Point));
			}
		}
	}

	for (ARGBColorPlatform* Platform : AssignedPlatforms) {
		if (!IsValid(Platform)) {
			continue;
		}

		if (URGBColorComponent* Color = Platform->GetColorComponent()) {
			Color->OnColorChanged.AddUniqueDynamic(this, &ARGBArenaControl::HandlePlatformColorChanged);
		}

		Platform->OnDestroyed.AddUniqueDynamic(this, &ARGBArenaControl::HandlePlatformDestroyed);
	}

	for (ARGBEnemyCharacter* Enemy : AssignedEnemies) {
		RecordEnemySpawn(Enemy);
	}
	AssignedEnemies.RemoveAll([this](const ARGBEnemyCharacter* Enemy) { return !IsValid(Enemy) || Enemy->GetOwningArena() != this; });

	if (IsValid(ArenaCameraZone))
	{
		ArenaCameraZone->OnPlayerEnteredZone.AddUniqueDynamic(this, &ARGBArenaControl::HandlePlayerEnteredArenaZone);
		ArenaCameraZone->OnPlayerExitedZone.AddUniqueDynamic(this, &ARGBArenaControl::HandlePlayerExitedArenaZone);
	}

	InitialEvaluationTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ARGBArenaControl::FinishArenaReset);

	if (UWorld* World = GetWorld()) {
		ActorSpawnedHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &ARGBArenaControl::HandleActorSpawned));
	}
}

void ARGBArenaControl::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(InitialEvaluationTimer);
	ClearEnemyRespawnTimers();

	if (IsValid(EnemyCoordinator)) {
		EnemyCoordinator->ResetCoordination();
	}

	if (ActorSpawnedHandle.IsValid()) {
		if (UWorld* World = GetWorld()) {
			World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
		}
		ActorSpawnedHandle.Reset();
	}

	for (ARGBColorPlatform* Platform : AssignedPlatforms) {
		if (!IsValid(Platform)) {
			continue;
		}

		if (URGBColorComponent* Color = Platform->GetColorComponent()) {
			Color->OnColorChanged.RemoveDynamic(this, &ARGBArenaControl::HandlePlatformColorChanged);
		}
		Platform->OnDestroyed.RemoveDynamic(this, &ARGBArenaControl::HandlePlatformDestroyed);
	}

	if (IsValid(ArenaCameraZone))
	{
		ArenaCameraZone->OnPlayerEnteredZone.RemoveDynamic(this, &ARGBArenaControl::HandlePlayerEnteredArenaZone);
		ArenaCameraZone->OnPlayerExitedZone.RemoveDynamic(this, &ARGBArenaControl::HandlePlayerExitedArenaZone);
	}

	for (ARGBEnemyCharacter* Enemy : AssignedEnemies) {
		if (IsValid(Enemy)) {
			Enemy->OnEnemyEliminated.RemoveDynamic(this, &ARGBArenaControl::HandleEnemyEliminated);
		}
	}

	for (TPair<int32, TObjectPtr<ARGBEnemyCharacter>>& PendingRespawn : PendingRespawnEnemies) {
		if (IsValid(PendingRespawn.Value)) {
			PendingRespawn.Value->DespawnForArenaCompletion();
		}
	}
	PendingRespawnEnemies.Reset();

	EnemySpawnRecordIndices.Reset();

	Super::EndPlay(EndPlayReason);
}

void ARGBArenaControl::HandlePlatformColorChanged(ERGBColor PreviousColor, ERGBColor NewColor)
{
	if (bResetInProgress || bCompleted) {
		return;
	}

	USoundBase* ResultSound = NewColor == RequiredColor ? CorrectColorSound.Get() : WrongColorSound.Get();

	if (IsValid(ResultSound)) {
		UGameplayStatics::PlaySoundAtLocation(this, ResultSound, GetActorLocation());
	}

	UpdateProgress();
}

void ARGBArenaControl::HandlePlatformDestroyed(AActor* DestroyedActor)
{
	for (TObjectPtr<ARGBColorPlatform>& Platform : AssignedPlatforms) {
		if (Platform.Get() == DestroyedActor) {
			Platform = nullptr;
		}
	}
	UpdateProgress();
}

void ARGBArenaControl::UpdateProgress() {

	if (bResetInProgress || bCompleted) {
		return;
	}

	MatchingPlatformCount = 0;
	CompletionPercentage = 0.0f;

	TSet<ARGBColorPlatform*> SeenPlatforms;
	bool bValidSetup = AssignedPlatforms.Num() > 0;

	const bool bValidColor = RequiredColor == ERGBColor::Red || RequiredColor == ERGBColor::Green || RequiredColor == ERGBColor::Blue;
	bValidSetup = bValidSetup && bValidColor;

	for (ARGBColorPlatform* Platform : AssignedPlatforms) {
		if (!IsValid(Platform) || SeenPlatforms.Contains(Platform)) {
			bValidSetup = false;
			continue;
		}

		SeenPlatforms.Add(Platform);

		URGBColorComponent* Color = Platform->GetColorComponent();
		
		if (!IsValid(Color)) {
			bValidSetup = false;
			continue;
		}

		if (Color->GetCurrentColor() == RequiredColor) {
			++MatchingPlatformCount;
		}
	}

	if (!bValidSetup) {
		MatchingPlatformCount = 0;
		CompletionPercentage = 0.0f;
		ApplyEnemyMovementSpeedMultiplier();

		ProgressText->SetText(FText::FromString(TEXT("Arena setup error - check assigned platforms")));
		UE_LOG(LogTemp, Warning, TEXT("%s: assign at least one platform, with no missing or ") TEXT("duplicate entries, and select a valid required color."), *GetName());
		OnArenaProgressChanged.Broadcast(0.0f);
		return;
	}

	const int32 TotalPlatforms = AssignedPlatforms.Num();

	CompletionPercentage = 100.0f * MatchingPlatformCount / TotalPlatforms;
	ApplyEnemyMovementSpeedMultiplier();

	OnArenaProgressChanged.Broadcast(GetCompletionFraction());

	const FString ColorName = StaticEnum<ERGBColor>()->GetNameStringByValue(static_cast<int64>(RequiredColor));

	ProgressText->SetText(FText::FromString(FString::Printf(TEXT("Progress: %.1f%%"), CompletionPercentage)));

	if (MatchingPlatformCount == TotalPlatforms) {
		CompleteArena();
	}
}

void ARGBArenaControl::CompleteArena()
{
	if (bCompleted || bResetInProgress) {
		return;
	}

	ClearEnemyRespawnTimers();

	if (IsValid(EnemyCoordinator)) {
		EnemyCoordinator->ResetCoordination();
	}

	bCompleted = true;

	if (IsValid(ArenaCompletedSound)) {
		UGameplayStatics::PlaySoundAtLocation(this, ArenaCompletedSound, GetActorLocation());
	}

	for (ARGBColorPlatform* Platform : AssignedPlatforms) {
		if (IsValid(Platform)) {
			if (URGBColorComponent* Color = Platform->GetColorComponent()) {
				Color->SetColorLocked(true);
			}
		}
	}

	for (ARGBEnemyCharacter* Enemy : AssignedEnemies) {
		if (IsValid(Enemy)) {
			Enemy->OnEnemyEliminated.RemoveDynamic(this, &ARGBArenaControl::HandleEnemyEliminated);
			const int32* RecordIndex = EnemySpawnRecordIndices.Find(Enemy);
			Enemy->SetEnemyCoordinator(nullptr);
			Enemy->SetArenaRespawnManaged(true);
			Enemy->PrepareForArenaRespawn();
			if (RecordIndex) {
				PendingRespawnEnemies.Add(*RecordIndex, Enemy);
			}
			else {
				Enemy->DespawnForArenaCompletion();
			}
		}
	}
	AssignedEnemies.Reset();

	EnemySpawnRecordIndices.Reset();

	ProgressText->SetText(FText::FromString(TEXT("Progress: 100.0% - Completed")));
	OnArenaCompleted.Broadcast();
}

void ARGBArenaControl::HandleActorSpawned(AActor* SpawnedActor)
{
	RegisterEnemyIfInsideArena(Cast<ARGBEnemyCharacter>(SpawnedActor));
}

void ARGBArenaControl::RegisterEnemyIfInsideArena(ARGBEnemyCharacter* Enemy)
{
	if (!IsValid(Enemy) || !IsValid(ArenaCameraZone) || !ArenaCameraZone->ContainWorldLocation(Enemy->GetActorLocation())) {
		return;
	}
	if (Enemy->GetOwningArena() && Enemy->GetOwningArena() != this) {
		return;
	}
	if (const ARGBArenaControl* SpawningArena = Cast<ARGBArenaControl>(Enemy->GetOwner()); SpawningArena && SpawningArena != this) {
		return;
	}

	if (bCompleted) {
		Enemy->DespawnForArenaCompletion();
		return;
	}

	AssignedEnemies.AddUnique(Enemy);

	if (!bRespawningEnemies) {
		RecordEnemySpawn(Enemy);
	}
}

void ARGBArenaControl::RecordEnemySpawn(ARGBEnemyCharacter* Enemy)
{
	if (!IsValid(Enemy)) {
		return;
	}
	if (Enemy->GetOwningArena() && Enemy->GetOwningArena() != this) {
		UE_LOG(LogTemp, Warning, TEXT("%s cannot register %s: already owned by arena %s."), *GetName(), *GetNameSafe(Enemy), *GetNameSafe(Enemy->GetOwningArena()));
		return;
	}

	const TSubclassOf<ARGBEnemyCharacter> EnemyClass = Enemy->GetClass();
	const FTransform SpawnTransform = Enemy->GetActorTransform();
	// Records belong to instances, including two configured enemies at the same transform.
	int32 SpawnRecordIndex = EnemySpawnRecordIndices.FindRef(Enemy);
	if (!EnemySpawnRecordIndices.Contains(Enemy)) { SpawnRecordIndex = INDEX_NONE; }

	if (SpawnRecordIndex == INDEX_NONE) {
		FRGBArenaEnemySpawnRecord NewRecord;
		NewRecord.EnemyClass = EnemyClass;
		NewRecord.SpawnTransform = SpawnTransform;
		SpawnRecordIndex = EnemySpawnRecord.Add(NewRecord);
	}

	TrackEnemySpawnRecord(Enemy, SpawnRecordIndex);
}

void ARGBArenaControl::RespawnRecordedEnemy()
{
	for (int32 SpawnRecordIndex = 0; SpawnRecordIndex < EnemySpawnRecord.Num(); ++SpawnRecordIndex) {
		RespawnEnemyFromRecord(SpawnRecordIndex);
	}
}

void ARGBArenaControl::HandlePlayerEnteredArenaZone()
{
	bPlayerInsideArena = true;

	for (ARGBEnemyCharacter* Enemy : AssignedEnemies) {
		if (IsValid(Enemy)) {
			Enemy->SetArenaAIActive(true);
		}
	}

	const float DisplayProgress = bCompleted ? 1.0f : GetCompletionFraction();
	OnArenaProgressChanged.Broadcast(DisplayProgress);
}

void ARGBArenaControl::HandlePlayerExitedArenaZone()
{
	bPlayerInsideArena = false;

	for (ARGBEnemyCharacter* Enemy : AssignedEnemies) {
		if (IsValid(Enemy)) {
			Enemy->SetArenaAIActive(false);
		}
	}

	OnArenaProgressChanged.Broadcast(0.0f);
}

void ARGBArenaControl::TrackEnemySpawnRecord(ARGBEnemyCharacter* Enemy, int32 SpawnRecordIndex)
{
	if (!IsValid(Enemy) || !EnemySpawnRecord.IsValidIndex(SpawnRecordIndex)) {
		return;
	}

	AssignedEnemies.AddUnique(Enemy);
	EnemySpawnRecordIndices.Add(Enemy, SpawnRecordIndex);
	Enemy->OnEnemyEliminated.AddUniqueDynamic(this, &ARGBArenaControl::HandleEnemyEliminated);
	Enemy->SetOwningArena(this);
	Enemy->SetArenaCameraZone(ArenaCameraZone);
	Enemy->SetEnemyCoordinator(EnemyCoordinator);
	Enemy->SetArenaRespawnManaged(bRespawnEliminatedEnemies);
	const FRGBEnemyArenaAttributes Attributes = CalculateEnemyAttributes(Enemy);
	Enemy->ApplyArenaAttributes(Attributes);
	Enemy->SetArenaMovementSpeedMultiplier(bUseArenaEnemyTuning ? Attributes.MovementSpeedMultiplier : CalculateEnemyMovementSpeedMultiplier());
	Enemy->SetArenaAIActive(bPlayerInsideArena);

	if (IsValid(EnemyCoordinator)) {
		EnemyCoordinator->RegisterEnemy(Enemy, Enemy->GetEnemyMobilityType());
	}
}

void ARGBArenaControl::HandleEnemyEliminated(ARGBEnemyCharacter* EliminatedEnemy)
{
	if (!IsValid(EliminatedEnemy)) {
		return;
	}

	const int32* FoundSpawnRecordIndex = EnemySpawnRecordIndices.Find(EliminatedEnemy);

	if (!FoundSpawnRecordIndex) {
		return;
	}

	const int32 SpawnRecordIndex = *FoundSpawnRecordIndex;

	if (IsValid(EnemyCoordinator)) {
		EnemyCoordinator->UnregisterEnemy(EliminatedEnemy);
	}

	EliminatedEnemy->SetEnemyCoordinator(nullptr);
	EliminatedEnemy->SetArenaCameraZone(nullptr);
	EliminatedEnemy->OnEnemyEliminated.RemoveDynamic(this, &ARGBArenaControl::HandleEnemyEliminated);
	EnemySpawnRecordIndices.Remove(EliminatedEnemy);
	AssignedEnemies.Remove(EliminatedEnemy);

	if (!bRespawnEliminatedEnemies || bCompleted || bResetInProgress || !EnemySpawnRecord.IsValidIndex(SpawnRecordIndex)) {
		EliminatedEnemy->SetArenaRespawnManaged(false);
		return;
	}

	EliminatedEnemy->PrepareForArenaRespawn();
	PendingRespawnEnemies.Add(SpawnRecordIndex, EliminatedEnemy);

	FTimerDelegate RespawnDelegate;
	RespawnDelegate.BindUObject(this, &ARGBArenaControl::RespawnEnemyFromRecord, SpawnRecordIndex);

	const float SafeRespawnDelay = CalculateEnemyRespawnDelay();

	if (SafeRespawnDelay <= SMALL_NUMBER) {
		EnemyRespawnTimers.Add(SpawnRecordIndex, GetWorldTimerManager().SetTimerForNextTick(RespawnDelegate));
		return;
	}

	FTimerHandle& RespawnTimer = EnemyRespawnTimers.FindOrAdd(SpawnRecordIndex);
	GetWorldTimerManager().SetTimer(RespawnTimer, RespawnDelegate, SafeRespawnDelay, false);
}

float ARGBArenaControl::CalculateEnemyRespawnDelay() const
{
	if (!bScaleEnemyRespawnDelayWithArenaProgress) {
		return FMath::Max(EnemyRespawnDelay, 0.0f);
	}

	const float DelayAtNoProgress = FMath::Max(EnemyRespawnDelayAtNoProgress, 0.0f);
	const float DelayNearCompletion = FMath::Max(EnemyRespawnDelayNearCompletion, 0.0f);
	return FMath::Lerp(DelayAtNoProgress, DelayNearCompletion, GetCompletionFraction());
}

float ARGBArenaControl::CalculateEnemyMovementSpeedMultiplier() const
{
	if (!bScaleEnemyMovementSpeedWithArenaProgress) {
		return 1.0f;
	}

	const float MultiplierAtNoProgress = FMath::Max(EnemyMovementSpeedMultiplierAtNoProgress, 0.0f);
	const float MultiplierNearCompletion = FMath::Max(EnemyMovementSpeedMultiplierNearCompletion, 0.0f);
	return FMath::Lerp(MultiplierAtNoProgress, MultiplierNearCompletion, GetCompletionFraction());
}

void ARGBArenaControl::ApplyEnemyMovementSpeedMultiplier()
{
	const float MovementSpeedMultiplier = CalculateEnemyMovementSpeedMultiplier();
	if (bUseArenaEnemyTuning && IsValid(EnemyCoordinator)) {
		CurrentMaximumAttackers = FMath::Max(0, FMath::FloorToInt(CoordinationTuning.MaximumAttackers.Evaluate(GetCompletionFraction())));
		EnemyCoordinator->SetMaximumAttackers(CurrentMaximumAttackers);
	}

	for (ARGBEnemyCharacter* Enemy : AssignedEnemies) {
		if (IsValid(Enemy) && (!Enemy->GetOwningArena() || Enemy->GetOwningArena() == this)) {
			const FRGBEnemyArenaAttributes Attributes = CalculateEnemyAttributes(Enemy);
			Enemy->ApplyArenaAttributes(Attributes);
			Enemy->SetArenaMovementSpeedMultiplier(bUseArenaEnemyTuning ? Attributes.MovementSpeedMultiplier : MovementSpeedMultiplier);
		}
	}
}

FRGBEnemyArenaAttributes ARGBArenaControl::CalculateEnemyAttributes(const ARGBEnemyCharacter* Enemy) const
{
	FRGBEnemyArenaAttributes Result;
	Result.bEnabled = bUseArenaEnemyTuning;
	Result.Completion = GetCompletionFraction();
	if (!bUseArenaEnemyTuning || !Enemy) {
		return Result;
	}

	const float Progress = GetCompletionFraction();
	const auto Positive = [Progress](const FRGBProgressValue& Value) { return FMath::Max(0.0f, Value.Evaluate(Progress)); };
	switch (Enemy->GetEnemyMobilityType()) {
	case ERGBEnemyMobilityType::Ground:
		Result.MovementSpeedMultiplier = Positive(GroundEnemyTuning.PatrolSpeedMultiplier);
		Result.ChaseSpeedMultiplier = Positive(GroundEnemyTuning.ChaseSpeedMultiplier);
		Result.ChasePreference = FMath::Clamp(GroundEnemyTuning.ChasePreference.Evaluate(Progress), 0.0f, 1.0f);
		Result.DetectionRange = Positive(GroundEnemyTuning.DetectionRange);
		Result.AttackCooldownMultiplier = Positive(GroundEnemyTuning.AttackCooldownMultiplier);
		Result.MinimumPatrolDirectionTime = FMath::Max(0.1f, GroundEnemyTuning.MinimumPatrolDirectionTime);
		Result.MaximumPatrolDirectionTime = FMath::Max(Result.MinimumPatrolDirectionTime, GroundEnemyTuning.MaximumPatrolDirectionTime);
		break;
	case ERGBEnemyMobilityType::Teleporter:
		Result.MinimumTeleportIdleTime = FMath::Max(0.1f, Positive(TeleporterEnemyTuning.MinimumIdleTime));
		Result.MaximumTeleportIdleTime = FMath::Max(Result.MinimumTeleportIdleTime, Positive(TeleporterEnemyTuning.MaximumIdleTime));
		Result.TeleportDangerRadius = Positive(TeleporterEnemyTuning.DangerRadius);
		Result.TeleportProximityWeight = FMath::Clamp(TeleporterEnemyTuning.PlayerProximityWeight.Evaluate(Progress), 0.0f, 1.0f);
		Result.TeleportHorizontalAttackRange = Positive(TeleporterEnemyTuning.HorizontalAttackRange);
		Result.TeleportSameLevelHeightTolerance = FMath::Max(TeleporterEnemyTuning.SameLevelHeightTolerance, 0.0f);
		Result.AttackCooldownMultiplier = Positive(TeleporterEnemyTuning.AttackCooldownMultiplier);
		break;
	case ERGBEnemyMobilityType::Flying:
		Result.MovementSpeedMultiplier = Positive(FlyingEnemyTuning.SpeedMultiplier);
		Result.FlightAccelerationMultiplier = Positive(FlyingEnemyTuning.AccelerationMultiplier);
		Result.FlightTrackingWeight = Positive(FlyingEnemyTuning.PlayerTrackingWeight);
		Result.FlightSeparationWeight = Positive(FlyingEnemyTuning.EnemySeparationWeight);
		Result.FlightSeparationRadius = Positive(FlyingEnemyTuning.EnemySeparationRadius);
		Result.FlightGeometryWeight = Positive(FlyingEnemyTuning.GeometryAvoidanceWeight);
		Result.FlightProbeDistance = Positive(FlyingEnemyTuning.GeometryProbeDistance);
		Result.AttackCooldownMultiplier = Positive(FlyingEnemyTuning.AttackCooldownMultiplier);
		break;
	}
	return Result;
}

void ARGBArenaControl::RespawnEnemyFromRecord(int32 SpawnRecordIndex)
{
	EnemyRespawnTimers.Remove(SpawnRecordIndex);
	ARGBEnemyCharacter* PendingEnemy = PendingRespawnEnemies.FindRef(SpawnRecordIndex);

	if (bCompleted || !EnemySpawnRecord.IsValidIndex(SpawnRecordIndex)) {
		if (IsValid(PendingEnemy)) {
			PendingEnemy->DespawnForArenaCompletion();
		}
		PendingRespawnEnemies.Remove(SpawnRecordIndex);
		return;
	}

	const FRGBArenaEnemySpawnRecord& Record = EnemySpawnRecord[SpawnRecordIndex];

	if (IsValid(PendingEnemy)) {
		PendingRespawnEnemies.Remove(SpawnRecordIndex);
		PendingEnemy->RestoreForArenaRespawn(Record.SpawnTransform);
		TrackEnemySpawnRecord(PendingEnemy, SpawnRecordIndex);
		return;
	}

	UWorld* World = GetWorld();

	if (!IsValid(World) || !Record.EnemyClass) {
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.TransformScaleMethod = ESpawnActorScaleMethod::OverrideRootScale;

	const bool bWasRespawningEnemies = bRespawningEnemies;
	bRespawningEnemies = true;
	ARGBEnemyCharacter* Enemy = World->SpawnActor<ARGBEnemyCharacter>(Record.EnemyClass, Record.SpawnTransform, SpawnParameters);
	bRespawningEnemies = bWasRespawningEnemies;

	TrackEnemySpawnRecord(Enemy, SpawnRecordIndex);
}

void ARGBArenaControl::ClearEnemyRespawnTimers()
{
	for (TPair<int32, FTimerHandle>& RespawnTimer : EnemyRespawnTimers) {
		GetWorldTimerManager().ClearTimer(RespawnTimer.Value);
	}

	EnemyRespawnTimers.Reset();
}

void ARGBArenaControl::ApplyDifficultyPreset()
{
	if (!bUseArenaEnemyTuning || !bUseDifficultyPresets) {
		return;
	}

	ERGBDifficulty Difficulty = ERGBDifficulty::Medium;
	if (const UGameInstance* GameInstance = GetGameInstance()) {
		if (const URGBGameSettingsSubsystem* Settings = GameInstance->GetSubsystem<URGBGameSettingsSubsystem>()) {
			Difficulty = Settings->GetDifficulty();
		}
	}

	const FRGBDifficultyPreset Preset = URGBDifficultySettings::Resolve(Difficulty, DifficultyOffset);
	CoordinationTuning = Preset.Coordination;
	GroundEnemyTuning = Preset.Ground;
	TeleporterEnemyTuning = Preset.Teleporter;
	FlyingEnemyTuning = Preset.Flying;
	bScaleEnemyRespawnDelayWithArenaProgress = true;
	EnemyRespawnDelayAtNoProgress = FMath::Max(Preset.EnemyRespawnDelay.AtNoProgress, 0.0f);
	EnemyRespawnDelayNearCompletion = FMath::Max(Preset.EnemyRespawnDelay.NearCompletion, 0.0f);

	UE_LOG(LogTemp, Log, TEXT("%s uses difficulty preset %s with offset %.2f."), *GetName(), *UEnum::GetValueAsString(Difficulty), DifficultyOffset);
}
