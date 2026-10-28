// Easy / Medium / Hard enemy presets, editable under Project Settings > Game > RGB Difficulty.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "../AI/RGBArenaEnemyTuning.h"
#include "RGBGameSettingsSubsystem.h"
#include "RGBDifficultySettings.generated.h"

/** One complete set of arena enemy pressure values. */
USTRUCT(BlueprintType)
struct RGB_API FRGBDifficultyPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	FRGBArenaCoordinationTuning Coordination;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	FRGBArenaGroundTuning Ground;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	FRGBArenaTeleporterTuning Teleporter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	FRGBArenaFlyingTuning Flying;

	/** Seconds before an eliminated enemy returns, from no progress to near completion. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty")
	FRGBProgressValue EnemyRespawnDelay = FRGBProgressValue(8.0f, 2.0f);
};

/**
 * Difficulty presets used by every arena that has Use Arena Enemy Tuning and Use Difficulty
 * Presets enabled. Medium matches the original default tuning values.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "RGB Difficulty"))
class RGB_API URGBDifficultySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	URGBDifficultySettings();

	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Presets")
	FRGBDifficultyPreset Easy;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Presets")
	FRGBDifficultyPreset Medium;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Presets")
	FRGBDifficultyPreset Hard;

	/**
	 * Preset for a difficulty shifted by an arena's offset (-1..1). Fractional positions blend
	 * between neighbouring presets (e.g. Medium - 0.5 = halfway between Easy and Medium).
	 */
	static FRGBDifficultyPreset Resolve(ERGBDifficulty Difficulty, float Offset);
};
