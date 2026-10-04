// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Platforms/RGBColorComponent.h"
#include "TimerManager.h"
#include "RGBArenaControl.generated.h"

class ARGBColorPlatform;
class USceneComponent;
class ARGBArenaCameraZone;
class UTextRenderComponent;
class ARGBEnemyCharacter;
class USoundBase;

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
	// Sets default values for this actor's properties
	ARGBArenaControl();

	void BeginArenaReset();
	void FinishArenaReset();

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

private:
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

	FTimerHandle InitialEvaluationTimer;

	void HandleActorSpawned(AActor* SpawnedActor);
	void RegisterEnemyIfInsideArena(ARGBEnemyCharacter* Enemy);

	FDelegateHandle ActorSpawnedHandle;

	void RecordEnemySpawn(ARGBEnemyCharacter* Enemy);
	void RespawnRecordedEnemy();

	UPROPERTY(Transient)
	TArray<FRGBArenaEnemySpawnRecord> EnemySpawnRecord;

	bool bRespawningEnemies = false;
	bool bEnemiesNeedRespawn = false;
};
