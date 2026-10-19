// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Platforms/RGBColorComponent.h"
#include "TimerManager.h"
#include "../AI/RGBArenaEnemyTuning.h"
#include "RGBArenaControl.generated.h"

class ARGBColorPlatform;
class USceneComponent;
class ARGBArenaCameraZone;
class UTextRenderComponent;
class ARGBEnemyCharacter;
class USoundBase;
class URGBArenaEnemyCoordinatorComponent;
class ATargetPoint;

USTRUCT()
struct FRGBArenaEnemySpawnRecord {
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<ARGBEnemyCharacter> EnemyClass;

	UPROPERTY()
	FTransform SpawnTransform = FTransform::Identity;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRGBArenaCompletedSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRGBArenaProgressChangedSignature, float, Progress);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRGBArenaResetStartedSignature);

UCLASS()
class RGB_API ARGBArenaControl : public AActor
{
	GENERATED_BODY()
	
public:	
    ARGBArenaCameraZone* GetArenaCameraZone() const { return ArenaCameraZone; }
	// Sets default values for this actor's properties
	ARGBArenaControl();

	void BeginArenaReset();
	void FinishArenaReset();
	bool UsesArenaEnemyTuning() const { return bUseArenaEnemyTuning; }
	const TArray<TObjectPtr<ATargetPoint>>& GetTeleportTargetPoints() const { return TeleportTargetPoints; }

	UFUNCTION(BlueprintPure, Category = "RGB|Arena")
	bool IsCompleted() const { return bCompleted; }

	UPROPERTY(BlueprintAssignable, Category = "RGB|Arena")
	FRGBArenaCompletedSignature OnArenaCompleted;

	UPROPERTY(BlueprintAssignable, Category = "RGB|Arena")
	FRGBArenaResetStartedSignature OnArenaResetStarted;

	UPROPERTY(BlueprintAssignable, Category = "RGB|Arena")
	FRGBArenaProgressChangedSignature OnArenaProgressChanged;

	UFUNCTION(BlueprintPure, Category = "RGB|Arena")
	float GetCompletionFraction() const {
		return FMath::Clamp(CompletionPercentage / 100.0f, 0.0f, 1.0f);
	}

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Arena")
	TObjectPtr<UTextRenderComponent> ProgressText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Arena")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Arena|Enemies")
	TObjectPtr<URGBArenaEnemyCoordinatorComponent> EnemyCoordinator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Arena")
	ERGBColor RequiredColor = ERGBColor::Blue;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Arena")
	TArray<TObjectPtr<ARGBColorPlatform>> AssignedPlatforms;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Arena")
	TArray<TObjectPtr<ARGBEnemyCharacter>> AssignedEnemies;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Arena")
	TObjectPtr<ARGBArenaCameraZone> ArenaCameraZone;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Arena|Audio")
	TObjectPtr<USoundBase> CorrectColorSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Arena|Audio")
	TObjectPtr<USoundBase> WrongColorSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Arena|Audio")
	TObjectPtr<USoundBase> ArenaCompletedSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Arena|Enemies")
	bool bRespawnEliminatedEnemies = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Arena|Enemies", meta = (EditCondition = "bRespawnEliminatedEnemies"))
	bool bScaleEnemyRespawnDelayWithArenaProgress = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Arena|Enemies", meta = (EditCondition = "bRespawnEliminatedEnemies && !bScaleEnemyRespawnDelayWithArenaProgress", EditConditionHides, ClampMin = "0.0", Units = "s"))
	float EnemyRespawnDelay = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Arena|Enemies", meta = (EditCondition = "bRespawnEliminatedEnemies && bScaleEnemyRespawnDelayWithArenaProgress", EditConditionHides, ClampMin = "0.0", Units = "s"))
	float EnemyRespawnDelayAtNoProgress = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Arena|Enemies", meta = (EditCondition = "bRespawnEliminatedEnemies && bScaleEnemyRespawnDelayWithArenaProgress", EditConditionHides, ClampMin = "0.0", Units = "s"))
	float EnemyRespawnDelayNearCompletion = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Arena|Enemies")
	bool bScaleEnemyMovementSpeedWithArenaProgress = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Arena|Enemies", meta = (EditCondition = "bScaleEnemyMovementSpeedWithArenaProgress", EditConditionHides, ClampMin = "0.0"))
	float EnemyMovementSpeedMultiplierAtNoProgress = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Arena|Enemies", meta = (EditCondition = "bScaleEnemyMovementSpeedWithArenaProgress", EditConditionHides, ClampMin = "0.0"))
	float EnemyMovementSpeedMultiplierNearCompletion = 1.5f;

	/** Enable the per-variant profiles below. These replace the older shared speed multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Arena|AI")
	bool bUseArenaEnemyTuning = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Arena|AI", meta = (EditCondition = "bUseArenaEnemyTuning"))
	FRGBArenaCoordinationTuning CoordinationTuning;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Arena|AI", meta = (EditCondition = "bUseArenaEnemyTuning"))
	FRGBArenaGroundTuning GroundEnemyTuning;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Arena|AI", meta = (EditCondition = "bUseArenaEnemyTuning"))
	FRGBArenaTeleporterTuning TeleporterEnemyTuning;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Arena|AI", meta = (EditCondition = "bUseArenaEnemyTuning"))
	FRGBArenaFlyingTuning FlyingEnemyTuning;

	/** Arena-owned points replace tag discovery when arena AI tuning is enabled. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Arena|AI")
	TArray<TObjectPtr<ATargetPoint>> TeleportTargetPoints;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "RGB|Arena|AI|Debug")
	int32 CurrentMaximumAttackers = 1;

private:
	friend class FRGBArenaLifecycleTest;
	UFUNCTION()
	void HandlePlatformColorChanged(ERGBColor PreviousColor, ERGBColor NewColor);

	UFUNCTION()
	void HandlePlatformDestroyed(AActor* DestroyedActor);

	void UpdateProgress();

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "RGB|Arena")
	int32 MatchingPlatformCount = 0;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "RGB|Arena")
	float CompletionPercentage = 0.0f;

	void CompleteArena();

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "RGB|Arena")
	bool bCompleted = false;

	bool bResetInProgress = true;
	bool bPlayerInsideArena = false;

	FTimerHandle InitialEvaluationTimer;

	void HandleActorSpawned(AActor* SpawnedActor);
	void RegisterEnemyIfInsideArena(ARGBEnemyCharacter* Enemy);

	FDelegateHandle ActorSpawnedHandle;

	void RecordEnemySpawn(ARGBEnemyCharacter* Enemy);
	void RespawnRecordedEnemy();

	UPROPERTY(Transient)
	TArray<FRGBArenaEnemySpawnRecord> EnemySpawnRecord;

	TMap<ARGBEnemyCharacter*, int32> EnemySpawnRecordIndices;
	TMap<int32, FTimerHandle> EnemyRespawnTimers;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<ARGBEnemyCharacter>> PendingRespawnEnemies;

	bool bRespawningEnemies = false;
	bool bEnemiesNeedRespawn = false;

	UFUNCTION()
	void HandlePlayerEnteredArenaZone();

	UFUNCTION()
	void HandlePlayerExitedArenaZone();

	UFUNCTION()
	void HandleEnemyEliminated(ARGBEnemyCharacter* EliminatedEnemy);

	void TrackEnemySpawnRecord(ARGBEnemyCharacter* Enemy, int32 SpawnRecordIndex);
	float CalculateEnemyRespawnDelay() const;
	float CalculateEnemyMovementSpeedMultiplier() const;
	void ApplyEnemyMovementSpeedMultiplier();
	FRGBEnemyArenaAttributes CalculateEnemyAttributes(const ARGBEnemyCharacter* Enemy) const;
	void RespawnEnemyFromRecord(int32 SpawnRecordIndex);
	void ClearEnemyRespawnTimers();
};
