// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RGBGameMode.generated.h"

/**
 * 
 */

class ARGBCheckpoint;
class APlayerController;

UCLASS()
class RGB_API ARGBGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ARGBGameMode();

	bool RespawnPlayer(AController* PlayerController);

	bool RestartMechanicsTest(AController* PlayerController);

	void SetActiveCheckpoint(ARGBCheckpoint* Checkpoint);

	void StartRespawnTransition(AController* PlayerController);

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<ARGBCheckpoint> ActiveCheckpoint;

	void CompleteRespawnTransition();

	UPROPERTY(EditDefaultsOnly, Category = "RGB|Respawn", meta = (ClampMin = "0.0"))
	float RespawnFadeOutDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "RGB|Respawn", meta = (ClampMin = "0.0"))
	float RespawnBlackDelay = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "RGB|Respawn", meta = (ClampMin = "0.0"))
	float RespawnFadeInDuration = 0.35f;

	UPROPERTY(Transient)
	TWeakObjectPtr<AController> PendingRespawnController;

	FTimerHandle RespawnTimerHandle;

	bool bRespawnTransitionActive = false;

};
