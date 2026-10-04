// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBGameMode.h"

#include "../Player/RGBPlayerCharacter.h"
#include "../Player/RGBPlayerController.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

#include "EngineUtils.h"
#include "../Platforms/RGBColorPlatform.h"
#include "../Platforms/RGBColorComponent.h"

#include "../Arena/RGBArenaControl.h"

#include "../Progression/RGBCheckpoint.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

ARGBGameMode::ARGBGameMode()
{
	DefaultPawnClass = ARGBPlayerCharacter::StaticClass();
	PlayerControllerClass = ARGBPlayerController::StaticClass();
}

bool ARGBGameMode::RespawnPlayer(AController* PlayerController)
{
	if (!IsValid(PlayerController)) {
		return false;
	}
	
	const ARGBCheckpoint* Checkpoint = ActiveCheckpoint.Get();
	AActor* PlayerStart = nullptr;

	if (!IsValid(Checkpoint)) {
		PlayerStart = FindPlayerStart(PlayerController);

		if (!IsValid(PlayerStart))
		{
			UE_LOG(LogTemp, Error, TEXT("Respawn failed: no checkpoint or Player Start found."));
			return false;
		}
	}

	if (APawn* OldPawn = PlayerController->GetPawn()) {
		PlayerController->UnPossess();
		OldPawn->Destroy();
	}

	if (IsValid(Checkpoint)) {
		RestartPlayerAtTransform(PlayerController, Checkpoint->GetRespawnTransform());
	}
	else {
		RestartPlayerAtPlayerStart(PlayerController, PlayerStart);
	}

	const bool bRespawnSucceeded = IsValid(PlayerController->GetPawn());

	if (!bRespawnSucceeded) {
		UE_LOG(LogTemp,Error, TEXT("Respawn failed: check pawn class and spawn clearance."));
	}
	return bRespawnSucceeded;
}

bool ARGBGameMode::RestartMechanicsTest(AController* PlayerController)
{
	if (!RespawnPlayer(PlayerController)) {
		return false;
	}

	TArray<TWeakObjectPtr<ARGBArenaControl>> Arenas;

	for (TActorIterator<ARGBArenaControl> It(GetWorld()); It; ++It) {
		ARGBArenaControl* Arena = *It;

		if (IsValid(Arena)) {
			Arenas.Add(TWeakObjectPtr<ARGBArenaControl>(Arena));
			Arena->BeginArenaReset();
		}
	}

	for (TActorIterator<ARGBColorPlatform> It(GetWorld()); It; ++It)
	{
		ARGBColorPlatform* Platform = *It;
		if (IsValid(Platform)) {
			if (URGBColorComponent* Color = Platform->GetColorComponent()) {
				Color->ResetColor();
			}
		}
	}

	for (const TWeakObjectPtr<ARGBArenaControl>& Arena : Arenas) {
		if (Arena.IsValid()) {
			Arena->FinishArenaReset();
		}
	}

	return true;
}

void ARGBGameMode::SetActiveCheckpoint(ARGBCheckpoint* Checkpoint)
{
	if (IsValid(Checkpoint)) {
		ActiveCheckpoint = Checkpoint;
	}
}

void ARGBGameMode::StartRespawnTransition(AController* PlayerController)
{
	if (bRespawnTransitionActive || !IsValid(PlayerController)) {
		return;
	}

	APlayerController* LocalPlayerController = Cast<APlayerController>(PlayerController);

	if (!IsValid(LocalPlayerController)) {
		RespawnPlayer(PlayerController);
		return;
	}

	bRespawnTransitionActive = true;
	PendingRespawnController = PlayerController;

	LocalPlayerController->SetIgnoreMoveInput(true);
	LocalPlayerController->SetIgnoreLookInput(true);

	if (IsValid(LocalPlayerController->PlayerCameraManager)) {
		LocalPlayerController->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, RespawnFadeOutDuration, FLinearColor::Black, false, true);
	}

	const float DelayBeforeRespawn = RespawnFadeOutDuration + RespawnBlackDelay;
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &ARGBGameMode::CompleteRespawnTransition, DelayBeforeRespawn, false);
}

void ARGBGameMode::CompleteRespawnTransition()
{
	AController* PlayerController = PendingRespawnController.Get();
	APlayerController* LocalPlayerController = Cast<ARGBPlayerController>(PlayerController);

	if (IsValid(PlayerController)) {
		RespawnPlayer(PlayerController);
	}

	if (IsValid(LocalPlayerController)) {
		if (IsValid(LocalPlayerController->PlayerCameraManager)) {
			LocalPlayerController->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, RespawnFadeInDuration, FLinearColor::Black, false, false);
		}

		LocalPlayerController->SetIgnoreMoveInput(false);
		LocalPlayerController->SetIgnoreLookInput(false);
	}

	PendingRespawnController.Reset();
	bRespawnTransitionActive = false;
}
