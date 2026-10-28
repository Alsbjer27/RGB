// Session-wide player settings chosen in the main menu (not saved to disk).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RGBGameSettingsSubsystem.generated.h"

class UWorld;

UENUM(BlueprintType)
enum class ERGBDifficulty : uint8
{
	Easy,
	Medium,
	Hard
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRGBGameSettingsChangedSignature);

/**
 * Holds the main-menu settings for the current game session. Values survive level changes and
 * respawns but reset when the game is closed. Master volume is applied to the world's audio
 * device and re-applied after every map load.
 */
UCLASS()
class RGB_API URGBGameSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "RGB|Settings")
	float GetMasterVolume() const { return MasterVolume; }

	/** 0..1. Applied immediately to all game audio. */
	UFUNCTION(BlueprintCallable, Category = "RGB|Settings")
	void SetMasterVolume(float NewVolume);

	UFUNCTION(BlueprintPure, Category = "RGB|Settings")
	ERGBDifficulty GetDifficulty() const { return Difficulty; }

	UFUNCTION(BlueprintCallable, Category = "RGB|Settings")
	void SetDifficulty(ERGBDifficulty NewDifficulty);

	UPROPERTY(BlueprintAssignable, Category = "RGB|Settings")
	FRGBGameSettingsChangedSignature OnSettingsChanged;

private:
	void ApplyMasterVolume() const;
	void HandlePostLoadMap(UWorld* LoadedWorld);

	float MasterVolume = 1.0f;
	ERGBDifficulty Difficulty = ERGBDifficulty::Medium;
	FDelegateHandle PostLoadMapHandle;
};
