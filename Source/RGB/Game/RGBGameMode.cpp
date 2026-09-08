// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBGameMode.h"

#include "../Player/RGBPlayerCharacter.h"
#include "../Player/RGBPlayerController.h"


ARGBGameMode::ARGBGameMode()
{
	DefaultPawnClass = ARGBPlayerCharacter::StaticClass();
	PlayerControllerClass = ARGBPlayerController::StaticClass();
}
