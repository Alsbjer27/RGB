#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RGBEnemyAITypes.h"
#include "RGBArenaEnemyCoordinatorComponent.generated.h"

class ACharacter;
class ARGBEnemyCharacter;

UCLASS(ClassGroup = (RGB), meta = (BlueprintSpawnableComponent))
class RGB_API URGBArenaEnemyCoordinatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URGBArenaEnemyCoordinatorComponent();

	void RegisterEnemy(ARGBEnemyCharacter* Enemy, ERGBEnemyMobilityType MobilityType = ERGBEnemyMobilityType::Ground);
	void UnregisterEnemy(ARGBEnemyCharacter* Enemy);
	void ResetCoordination();
	void SetPlayerTarget(ACharacter* NewPlayerTarget);
	void ConfigureArenaPolicy(float DecisionInterval, float RoleCommitment, int32 MaximumAttackers);
	void SetMaximumAttackers(int32 MaximumAttackers);

	bool GetAssignment(const ARGBEnemyCharacter* Enemy, FRGBEnemyCoordinationAssignment& OutAssignment) const;
	bool CanEnemyAttack(const ARGBEnemyCharacter* Enemy) const;
	bool TryAcquireAttackPermission(ARGBEnemyCharacter* Enemy);
	void ReleaseAttackPermission(ARGBEnemyCharacter* Enemy);
	bool IsTargetReservedByOther(const ARGBEnemyCharacter* Enemy, const AActor* Target) const;
	bool TryReserveTarget(ARGBEnemyCharacter* Enemy, AActor* Target);
	void ReleaseTargetReservation(ARGBEnemyCharacter* Enemy, const AActor* Target = nullptr);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|AI|Coordination", meta = (ClampMin = "0.05", Units = "s"))
	float AssignmentUpdateInterval = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|AI|Coordination", meta = (ClampMin = "1"))
	int32 MaximumSimultaneousAttackers = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|AI|Coordination", meta = (ClampMin = "0.0", Units = "s"))
	float MinimumAssignmentCommitment = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|AI|Coordination", meta = (ClampMin = "0.1", Units = "s"))
	float AssignmentLeaseDuration = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|AI|Coordination", meta = (ClampMin = "0.0", Units = "cm"))
	float MeleeSlotDistance = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|AI|Coordination", meta = (ClampMin = "0.0", Units = "cm"))
	float PressureSlotDistance = 280.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|AI|Coordination", meta = (ClampMin = "0.0", Units = "cm"))
	float AdditionalPressureSlotSpacing = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|AI|Coordination", meta = (ClampMin = "0.0", Units = "cm"))
	float FlyingVerticalSlotSpacing = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|AI|Debug")
	bool bEnableDebugLogging = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "RGB|AI|Debug")
	int32 RegisteredEnemyCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "RGB|AI|Debug")
	int32 ActiveAttackPermissionCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "RGB|AI|Debug")
	int32 ActiveTargetReservationCount = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "RGB|AI|Debug")
	TArray<FString> CurrentAssignments;

private:
	struct FRegisteredEnemy
	{
		TWeakObjectPtr<ARGBEnemyCharacter> Enemy;
		ERGBEnemyMobilityType MobilityType = ERGBEnemyMobilityType::Ground;
		FRGBEnemyCoordinationAssignment Assignment;
		double CommitmentExpiresAt = 0.0;
		bool bAttackPermissionAcquired = false;
		bool bWantsChase = true;
		double ChaseDecisionExpiresAt = 0.0;
	};

	struct FTargetReservation
	{
		TWeakObjectPtr<AActor> Target;
		TWeakObjectPtr<ARGBEnemyCharacter> Enemy;
	};

	void UpdateAssignments();
	void RemoveInvalidEnemies();
	FRegisteredEnemy* FindRegisteredEnemy(const ARGBEnemyCharacter* Enemy);
	const FRegisteredEnemy* FindRegisteredEnemy(const ARGBEnemyCharacter* Enemy) const;
	FVector MakeMeleeSlotLocation(const FRegisteredEnemy& Record, const FVector& PlayerLocation) const;
	FVector MakePressureSlotLocation(const FRegisteredEnemy& Record, const FVector& PlayerLocation, int32 PressureIndex) const;
	void ApplyAssignment(FRegisteredEnemy& Record, ERGBEnemyCoordinationRole Role, const FVector& TargetLocation, int32 SlotIndex, bool bCanAttack, double CurrentTime);
	void RefreshDebugCounters();

	UPROPERTY(Transient)
	TWeakObjectPtr<ACharacter> PlayerTarget;

	TArray<FRegisteredEnemy> RegisteredEnemies;
	TArray<FTargetReservation> TargetReservations;
	FTimerHandle AssignmentTimer;
	bool bUseArenaPolicy = false;
};
