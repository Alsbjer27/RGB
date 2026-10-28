// Easy / Medium / Hard enemy presets, editable under Project Settings > Game > RGB Difficulty.

#include "RGBDifficultySettings.h"
#include "UObject/UnrealType.h"

namespace
{
	/** Blends every numeric field of a reflected struct; non-numeric fields come from the nearer side. */
	void LerpStruct(const UStruct* Struct, void* Out, const void* A, const void* B, float Alpha)
	{
		for (TFieldIterator<FProperty> It(Struct); It; ++It) {
			FProperty* Property = *It;
			void* OutValue = Property->ContainerPtrToValuePtr<void>(Out);
			const void* AValue = Property->ContainerPtrToValuePtr<void>(A);
			const void* BValue = Property->ContainerPtrToValuePtr<void>(B);
			if (const FFloatProperty* Float = CastField<FFloatProperty>(Property)) {
				Float->SetPropertyValue(OutValue, FMath::Lerp(Float->GetPropertyValue(AValue), Float->GetPropertyValue(BValue), Alpha));
			}
			else if (const FDoubleProperty* Double = CastField<FDoubleProperty>(Property)) {
				Double->SetPropertyValue(OutValue, FMath::Lerp(Double->GetPropertyValue(AValue), Double->GetPropertyValue(BValue), static_cast<double>(Alpha)));
			}
			else if (const FIntProperty* Int = CastField<FIntProperty>(Property)) {
				Int->SetPropertyValue(OutValue, FMath::RoundToInt(FMath::Lerp(static_cast<float>(Int->GetPropertyValue(AValue)), static_cast<float>(Int->GetPropertyValue(BValue)), Alpha)));
			}
			else if (const FStructProperty* Nested = CastField<FStructProperty>(Property)) {
				LerpStruct(Nested->Struct, OutValue, AValue, BValue, Alpha);
			}
			else {
				Property->CopyCompleteValue(OutValue, Alpha < 0.5f ? AValue : BValue);
			}
		}
	}

	FRGBProgressValue PV(float AtNoProgress, float NearCompletion)
	{
		return FRGBProgressValue(AtNoProgress, NearCompletion);
	}
}

URGBDifficultySettings::URGBDifficultySettings()
{
	// Medium keeps the struct defaults, i.e. the tuning the arenas used before presets existed.

	// Easy: fewer simultaneous attackers, slower and calmer enemies, longer cooldowns and respawns.
	Easy.Coordination.MaximumAttackers = PV(1.0f, 2.0f);
	Easy.Ground.PatrolSpeedMultiplier = PV(0.85f, 1.1f);
	Easy.Ground.ChaseSpeedMultiplier = PV(1.2f, 1.6f);
	Easy.Ground.ChasePreference = PV(0.25f, 0.7f);
	Easy.Ground.DetectionRange = PV(650.0f, 1100.0f);
	Easy.Ground.AttackCooldownMultiplier = PV(1.3f, 0.9f);
	Easy.Teleporter.MinimumIdleTime = PV(2.8f, 1.0f);
	Easy.Teleporter.MaximumIdleTime = PV(7.5f, 3.0f);
	Easy.Teleporter.DangerRadius = PV(800.0f, 450.0f);
	Easy.Teleporter.PlayerProximityWeight = PV(0.2f, 0.6f);
	Easy.Teleporter.HorizontalAttackRange = PV(220.0f, 280.0f);
	Easy.Teleporter.AttackCooldownMultiplier = PV(1.3f, 1.0f);
	Easy.Flying.SpeedMultiplier = PV(0.8f, 1.25f);
	Easy.Flying.AccelerationMultiplier = PV(0.85f, 1.2f);
	Easy.Flying.PlayerTrackingWeight = PV(2.0f, 3.0f);
	Easy.Flying.AttackCooldownMultiplier = PV(1.3f, 1.0f);
	Easy.EnemyRespawnDelay = PV(11.0f, 4.0f);

	// Hard: more simultaneous attackers, faster and more focused enemies, shorter cooldowns and respawns.
	Hard.Coordination.MaximumAttackers = PV(2.0f, 4.0f);
	Hard.Ground.PatrolSpeedMultiplier = PV(1.1f, 1.45f);
	Hard.Ground.ChaseSpeedMultiplier = PV(1.6f, 2.3f);
	Hard.Ground.ChasePreference = PV(0.5f, 1.0f);
	Hard.Ground.DetectionRange = PV(1000.0f, 1600.0f);
	Hard.Ground.AttackCooldownMultiplier = PV(0.85f, 0.5f);
	Hard.Teleporter.MinimumIdleTime = PV(1.5f, 0.4f);
	Hard.Teleporter.MaximumIdleTime = PV(4.5f, 1.5f);
	Hard.Teleporter.DangerRadius = PV(600.0f, 250.0f);
	Hard.Teleporter.PlayerProximityWeight = PV(0.4f, 1.0f);
	Hard.Teleporter.HorizontalAttackRange = PV(280.0f, 400.0f);
	Hard.Teleporter.AttackCooldownMultiplier = PV(0.85f, 0.55f);
	Hard.Flying.SpeedMultiplier = PV(1.15f, 1.85f);
	Hard.Flying.AccelerationMultiplier = PV(1.15f, 1.7f);
	Hard.Flying.PlayerTrackingWeight = PV(3.0f, 4.5f);
	Hard.Flying.AttackCooldownMultiplier = PV(0.85f, 0.6f);
	Hard.EnemyRespawnDelay = PV(6.0f, 1.5f);
}

FRGBDifficultyPreset URGBDifficultySettings::Resolve(ERGBDifficulty Difficulty, float Offset)
{
	const URGBDifficultySettings* Settings = GetDefault<URGBDifficultySettings>();
	const FRGBDifficultyPreset* Presets[] = { &Settings->Easy, &Settings->Medium, &Settings->Hard };

	const float Position = FMath::Clamp(static_cast<float>(Difficulty) + (FMath::IsFinite(Offset) ? Offset : 0.0f), 0.0f, 2.0f);
	const int32 Lower = FMath::Clamp(FMath::FloorToInt(Position), 0, 2);
	const int32 Upper = FMath::Min(Lower + 1, 2);
	const float Alpha = Position - static_cast<float>(Lower);

	FRGBDifficultyPreset Result = *Presets[Lower];
	if (Upper != Lower && Alpha > KINDA_SMALL_NUMBER) {
		LerpStruct(FRGBDifficultyPreset::StaticStruct(), &Result, Presets[Lower], Presets[Upper], Alpha);
	}
	return Result;
}
