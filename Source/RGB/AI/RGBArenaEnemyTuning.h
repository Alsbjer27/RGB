#pragma once

#include "CoreMinimal.h"
#include "RGBArenaEnemyTuning.generated.h"

class UCurveFloat;

namespace RGBEnemyTuning
{
	RGB_API float ScoreTeleportDanger(float Distance, float MinimumDistance, float MaximumDistance, float DangerRadius);
	RGB_API bool IsWithinHorizontalAttackRange(const FVector& EnemyLocation, const FVector& PlayerLocation, float Range);
	RGB_API bool IsSameCombatLevel(float EnemyFeetZ, float PlayerFeetZ, float HeightTolerance);
}

/** A designer-authored value evaluated against actual arena completion (0..1). */
USTRUCT(BlueprintType)
struct RGB_API FRGBProgressValue
{
	GENERATED_BODY()

	FRGBProgressValue() = default;
	FRGBProgressValue(float InStart, float InEnd) : AtNoProgress(InStart), NearCompletion(InEnd) {}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progress")
	float AtNoProgress = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progress")
	float NearCompletion = 1.0f;

	/** Optional normalized response: X = completion, Y = blend between the two values. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progress")
	TObjectPtr<UCurveFloat> ResponseCurve;

	float Evaluate(float Completion) const;
};

USTRUCT(BlueprintType)
struct RGB_API FRGBArenaCoordinationTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coordination")
	FRGBProgressValue MaximumAttackers = FRGBProgressValue(1.0f, 3.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coordination", meta = (ClampMin = "0.05", Units = "s"))
	float DecisionInterval = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Coordination", meta = (ClampMin = "0.0", Units = "s"))
	float MinimumRoleCommitment = 1.0f;
};

USTRUCT(BlueprintType)
struct RGB_API FRGBArenaGroundTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ground")
	FRGBProgressValue PatrolSpeedMultiplier = FRGBProgressValue(1.0f, 1.3f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ground")
	FRGBProgressValue ChaseSpeedMultiplier = FRGBProgressValue(1.4f, 2.0f);

	/** Probability of joining pursuit at a decision interval; retained during role commitment. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ground")
	FRGBProgressValue ChasePreference = FRGBProgressValue(0.35f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ground")
	FRGBProgressValue DetectionRange = FRGBProgressValue(800.0f, 1400.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ground")
	FRGBProgressValue AttackCooldownMultiplier = FRGBProgressValue(1.0f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ground", meta = (ClampMin = "0.1", Units = "s"))
	float MinimumPatrolDirectionTime = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ground", meta = (ClampMin = "0.1", Units = "s"))
	float MaximumPatrolDirectionTime = 4.0f;
};

USTRUCT(BlueprintType)
struct RGB_API FRGBArenaTeleporterTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleporter")
	FRGBProgressValue MinimumIdleTime = FRGBProgressValue(2.0f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleporter")
	FRGBProgressValue MaximumIdleTime = FRGBProgressValue(6.0f, 2.0f);

	/** Points inside this X/Z radius receive an additional danger score. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleporter")
	FRGBProgressValue DangerRadius = FRGBProgressValue(700.0f, 300.0f);

	/** 0 = random destination choice, 1 = strongest preference for danger/proximity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleporter")
	FRGBProgressValue PlayerProximityWeight = FRGBProgressValue(0.3f, 0.9f);

	/** Horizontal trigger distance after the separate same-level eligibility check. Damage still uses AoERadius. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleporter")
	FRGBProgressValue HorizontalAttackRange = FRGBProgressValue(250.0f, 350.0f);

	/** Maximum difference between feet heights for the same combat level; this is not a distance-scoring weight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleporter", meta = (ClampMin = "0.0", Units = "cm"))
	float SameLevelHeightTolerance = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Teleporter")
	FRGBProgressValue AttackCooldownMultiplier = FRGBProgressValue(1.0f, 0.7f);
};

USTRUCT(BlueprintType)
struct RGB_API FRGBArenaFlyingTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying")
	FRGBProgressValue SpeedMultiplier = FRGBProgressValue(1.0f, 1.6f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying")
	FRGBProgressValue AccelerationMultiplier = FRGBProgressValue(1.0f, 1.5f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying")
	FRGBProgressValue PlayerTrackingWeight = FRGBProgressValue(2.5f, 4.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying")
	FRGBProgressValue EnemySeparationWeight = FRGBProgressValue(1.75f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying")
	FRGBProgressValue EnemySeparationRadius = FRGBProgressValue(180.0f, 130.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying")
	FRGBProgressValue GeometryAvoidanceWeight = FRGBProgressValue(2.5f, 2.5f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying")
	FRGBProgressValue GeometryProbeDistance = FRGBProgressValue(240.0f, 320.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Flying")
	FRGBProgressValue AttackCooldownMultiplier = FRGBProgressValue(1.0f, 0.75f);
};

/** Read-only effective settings sent by the owning arena; authored enemy values stay intact. */
USTRUCT(BlueprintType)
struct RGB_API FRGBEnemyArenaAttributes
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	bool bEnabled = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float Completion = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float MovementSpeedMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float ChaseSpeedMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float ChasePreference = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float DetectionRange = 800.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float AttackCooldownMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float MinimumPatrolDirectionTime = 1.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float MaximumPatrolDirectionTime = 4.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float MinimumTeleportIdleTime = 2.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float MaximumTeleportIdleTime = 6.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float TeleportDangerRadius = 700.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float TeleportProximityWeight = 0.3f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float TeleportHorizontalAttackRange = 250.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float TeleportSameLevelHeightTolerance = 60.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float FlightAccelerationMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float FlightTrackingWeight = 2.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float FlightSeparationWeight = 1.75f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float FlightSeparationRadius = 180.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float FlightGeometryWeight = 2.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena AI")
	float FlightProbeDistance = 240.0f;
};
