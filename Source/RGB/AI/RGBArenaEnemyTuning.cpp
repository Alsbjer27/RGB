#include "RGBArenaEnemyTuning.h"

#include "Curves/CurveFloat.h"

float RGBEnemyTuning::ScoreTeleportDanger(float Distance, float MinimumDistance, float MaximumDistance, float DangerRadius)
{
	const float DistanceRange = MaximumDistance - MinimumDistance;
	const float Proximity = DistanceRange > SMALL_NUMBER ? 1.0f - FMath::Clamp((Distance - MinimumDistance) / DistanceRange, 0.0f, 1.0f) : 1.0f;
	const float RadiusScore = DangerRadius > SMALL_NUMBER ? 1.0f - FMath::Clamp(Distance / DangerRadius, 0.0f, 1.0f) : Proximity;
	return 0.5f * (Proximity + RadiusScore);
}

bool RGBEnemyTuning::IsWithinHorizontalAttackRange(const FVector& EnemyLocation, const FVector& PlayerLocation, float Range)
{
	return Range > 0.0f && FMath::Abs(EnemyLocation.X - PlayerLocation.X) <= Range;
}

bool RGBEnemyTuning::IsSameCombatLevel(float EnemyFeetZ, float PlayerFeetZ, float HeightTolerance)
{
	return FMath::IsFinite(EnemyFeetZ) && FMath::IsFinite(PlayerFeetZ) && FMath::Abs(EnemyFeetZ - PlayerFeetZ) <= FMath::Max(HeightTolerance, 0.0f);
}

float FRGBProgressValue::Evaluate(float Completion) const
{
	const float SafeCompletion = FMath::Clamp(FMath::IsFinite(Completion) ? Completion : 0.0f, 0.0f, 1.0f);
	const float CurveValue = IsValid(ResponseCurve) ? ResponseCurve->GetFloatValue(SafeCompletion) : SafeCompletion;
	const float Alpha = FMath::Clamp(FMath::IsFinite(CurveValue) ? CurveValue : SafeCompletion, 0.0f, 1.0f);
	return FMath::Lerp(AtNoProgress, NearCompletion, Alpha);
}
