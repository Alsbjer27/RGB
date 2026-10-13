// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBPlayerController.h"

#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Engine/World.h"
#include "../Game/RGBGameMode.h"

#include "../Camera/RGBPlayerCameraManager.h"

#include "Camera/PlayerCameraManager.h"

#include "TimerManager.h"

ARGBPlayerController::ARGBPlayerController(){
	PlayerCameraManagerClass = ARGBPlayerCameraManager::StaticClass();
}

void ARGBPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController() && IsValid(PlayerCameraManager))
	{
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
		PlayerCameraManager->SetManualCameraFade(1.0f, FLinearColor::Black, false);

		if (InitialBlackDelay > 0.0f)
		{
			GetWorldTimerManager().SetTimer(InitialBlackDelayTimerHandle, this, &ARGBPlayerController::StartInitialFadeIn, InitialBlackDelay, false);
		}
		else
		{
			StartInitialFadeIn();
		}
	}

	if (!IsLocalController()) {
		return;
	}

	ULocalPlayer* LocalPlayer = GetLocalPlayer();

	if (!ensureMsgf(LocalPlayer, TEXT("RGB requires a local player"))) {
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

    if (!ensureMsgf(InputSubsystem, TEXT("Enhanced Input subsystem is unavailable.")))
    {
        return;
    }

    if (!ensureMsgf(DefaultMappingContext, TEXT("DefaultMappingContext is not assigned.")))
    {
        return;
    }

	InputSubsystem->AddMappingContext(DefaultMappingContext, 0);

	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;
}

void ARGBPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(InitialBlackDelayTimerHandle);
	GetWorldTimerManager().ClearTimer(InitialFadeInTimerHandle);

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer()) {
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

		if (InputSubsystem && DefaultMappingContext) {
			InputSubsystem->RemoveMappingContext(DefaultMappingContext);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ARGBPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);

	if (!ensureMsgf(EnhancedInput, TEXT("RGBPlayerController requires Enhanced Input"))) {
		return;
	}

	if (ensureMsgf(RestartAction, TEXT("RestartAction is not assigned.")))
	{
		EnhancedInput->BindAction(RestartAction, ETriggerEvent::Started, this, &ARGBPlayerController::RestartPlayer);
	}
}

void ARGBPlayerController::RestartPlayer() {
	ARGBGameMode* GameMode = GetWorld()->GetAuthGameMode<ARGBGameMode>();

	if (ensureMsgf(GameMode, TEXT("Restart requires RGBGameMode"))) {
		GameMode->RestartMechanicsTest(this);
	}
}

void ARGBPlayerController::StartInitialFadeIn()
{
	if (IsValid(PlayerCameraManager))
	{
		PlayerCameraManager->StartCameraFade(1.0f, 0.0f, InitialFadeInDuration, FLinearColor::Black, false, false);
	}

	if (InitialFadeInDuration > 0.0f)
	{
		GetWorldTimerManager().SetTimer(InitialFadeInTimerHandle, this, &ARGBPlayerController::FinishInitialFadeIn, InitialFadeInDuration, false);
	}
	else
	{
		FinishInitialFadeIn();
	}
}

void ARGBPlayerController::FinishInitialFadeIn()
{
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
}