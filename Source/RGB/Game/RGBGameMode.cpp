// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBGameMode.h"

#include "../Player/RGBPlayerCharacter.h"
#include "../Player/RGBPlayerController.h"

#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

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

	AActor* SpawnPoint = FindPlayerStart(PlayerController);

	if (!IsValid(SpawnPoint)) {
		UE_LOG(LogTemp, Error, TEXT("Respawn failed: no Player Start found."));
		return false;
	}

	if (APawn* OldPawn = PlayerController->GetPawn()) {
		PlayerController->UnPossess();
		OldPawn->Destroy();
	}

	RestartPlayerAtPlayerStart(PlayerController, SpawnPoint);

	const bool bRespawnSucceeded = IsValid(PlayerController->GetPawn());

	if (!bRespawnSucceeded)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Respawn failed: check the pawn class and spawn clearance."));
	}

	return bRespawnSucceeded;
}
