// Session-wide player settings chosen in the main menu (not saved to disk).

#include "RGBGameSettingsSubsystem.h"
#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"

void URGBGameSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &URGBGameSettingsSubsystem::HandlePostLoadMap);
	ApplyMasterVolume();
}

void URGBGameSettingsSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	Super::Deinitialize();
}

void URGBGameSettingsSubsystem::SetMasterVolume(float NewVolume)
{
	const float ClampedVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);
	if (FMath::IsNearlyEqual(ClampedVolume, MasterVolume)) {
		return;
	}
	MasterVolume = ClampedVolume;
	ApplyMasterVolume();
	OnSettingsChanged.Broadcast();
}

void URGBGameSettingsSubsystem::SetDifficulty(ERGBDifficulty NewDifficulty)
{
	if (Difficulty == NewDifficulty) {
		return;
	}
	Difficulty = NewDifficulty;
	OnSettingsChanged.Broadcast();
}

void URGBGameSettingsSubsystem::ApplyMasterVolume() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
	if (World) {
		if (FAudioDeviceHandle AudioDevice = World->GetAudioDevice()) {
			AudioDevice->SetTransientPrimaryVolume(MasterVolume);
			return;
		}
	}
	if (GEngine) {
		if (FAudioDeviceHandle MainDevice = GEngine->GetMainAudioDevice()) {
			MainDevice->SetTransientPrimaryVolume(MasterVolume);
		}
	}
}

void URGBGameSettingsSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	ApplyMasterVolume();
}
