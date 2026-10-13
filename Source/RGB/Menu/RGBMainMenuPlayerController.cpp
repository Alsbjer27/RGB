// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBMainMenuPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "RGBMainMenuDirector.h"
#include "Kismet/KismetSystemLibrary.h"

#include "Kismet/GameplayStatics.h"

#include "Camera/PlayerCameraManager.h"
#include "TimerManager.h"

ARGBMainMenuPlayerController::ARGBMainMenuPlayerController()
{
	bShowMouseCursor = false;
}

void ARGBMainMenuPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);

	if (!IsValid(EnhancedInput)) {
		UE_LOG(LogTemp, Error, TEXT("%s requires an EnhancedInputComponent."), *GetName());
		return;
	}

	if (IsValid(NavigateAction)) {
		EnhancedInput->BindAction(NavigateAction, ETriggerEvent::Triggered, this, &ARGBMainMenuPlayerController::HandleNavigate);
		EnhancedInput->BindAction(NavigateAction, ETriggerEvent::Completed, this, &ARGBMainMenuPlayerController::HandleNavigateReleased);
		EnhancedInput->BindAction(NavigateAction, ETriggerEvent::Canceled, this, &ARGBMainMenuPlayerController::HandleNavigateReleased);
	}

	if (IsValid(ConfirmAction)) {
		EnhancedInput->BindAction(ConfirmAction, ETriggerEvent::Started, this, &ARGBMainMenuPlayerController::HandleConfirm);
	}
}

void ARGBMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<ARGBMainMenuDirector> It(GetWorld()); It; ++It) {
		MenuDirector = *It;
		break;
	}

	if (MenuDirector.IsValid())
	{
		MenuDirector->OnStartReady.AddDynamic(this, &ARGBMainMenuPlayerController::HandleStartReady);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s could not find an RGBMainMenuDirector in the level."), *GetName());
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer()) {
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) {
			if (IsValid(MenuMappingContext)) {
				InputSubsystem->AddMappingContext(MenuMappingContext, 0);
			}
			else {
				UE_LOG(LogTemp, Warning, TEXT("%s has no MenuMappingContext assigned."), *GetName());
			}
		}
	}
	SetInputMode(FInputModeGameOnly());
}

void ARGBMainMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MenuDirector.IsValid())
	{
		MenuDirector->OnStartReady.RemoveDynamic(this, &ARGBMainMenuPlayerController::HandleStartReady);
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer()) {
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (IsValid(MenuMappingContext))
			{
				InputSubsystem->RemoveMappingContext(MenuMappingContext);
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}

void ARGBMainMenuPlayerController::HandleNavigate(const FInputActionValue& Value)
{
	if (bLevelTransitionActive) {
		return;
	}

	if (MenuDirector.IsValid() &&
		MenuDirector->IsSequenceRunning())
	{
		return;
	}

	const float Direction = Value.Get<float>();

	if (bNavigationHeld || FMath::Abs(Direction) < NavigationThreshold) {
		return;
	}

	bNavigationHeld = true;

	const int32 CurrentIndex = static_cast<int32>(SelectedOption);
	const int32 DirectionStep = Direction > 0.0f ? -1 : 1;
	const int32 NewIndex = FMath::Clamp(CurrentIndex + DirectionStep, 0, static_cast<int32>(ERGBMainMenuOption::Exit));
	
	SetSelectedOption(static_cast<ERGBMainMenuOption>(NewIndex));
}

void ARGBMainMenuPlayerController::HandleNavigateReleased(const FInputActionValue& Value)
{
	bNavigationHeld = false;
}

void ARGBMainMenuPlayerController::HandleConfirm(const FInputActionValue& Value)
{
	if (bLevelTransitionActive) {
		return;
	}

	if (MenuDirector.IsValid() &&
		MenuDirector->IsSequenceRunning())
	{
		return;
	}

	switch (SelectedOption)
	{
	case ERGBMainMenuOption::Start:
		if (MenuDirector.IsValid())
		{
			MenuDirector->StartStartSequence();
		}
		break;

	case ERGBMainMenuOption::Settings:
		if (MenuDirector.IsValid())
		{
			MenuDirector->StartSettingsSequence();
		}
		break;

	case ERGBMainMenuOption::Credits:
		if (MenuDirector.IsValid())
		{
			MenuDirector->StartCreditsSequence();
		}
		break;

	case ERGBMainMenuOption::Exit:
		UKismetSystemLibrary::QuitGame(
			this,
			this,
			EQuitPreference::Quit,
			false);
		break;

	default:
		break;
	}
}

void ARGBMainMenuPlayerController::SetSelectedOption(ERGBMainMenuOption NewSelection)
{
	if (SelectedOption == NewSelection) {
		return;
	}

	SelectedOption = NewSelection;
	OnSelectionChanged.Broadcast(SelectedOption);
	UE_LOG(LogTemp, Display, TEXT("Main menu selection changed to index %d."), static_cast<int32>(SelectedOption));
}

void ARGBMainMenuPlayerController::HandleStartReady()
{
	if (bLevelTransitionActive) {
		return;
	}

	if (GameplayLevelName.IsNone()) {
		UE_LOG(LogTemp, Warning, TEXT("%s cannot start the game because GameplayLevelName is not assigned."), *GetName());
		return;
	}

	bLevelTransitionActive = true;

	OnLevelTransitionStarted.Broadcast();

	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	if (IsValid(PlayerCameraManager)) {
		PlayerCameraManager->StartCameraFade(0.0f, 1.0f, LevelFadeOutDuration, FLinearColor::Black, false, true);
	}

	const float TransistionDelay = LevelFadeOutDuration + LevelBlackDelay;

	GetWorldTimerManager().SetTimer(LevelTransitionTimerHandle, this, &ARGBMainMenuPlayerController::OpenGameplayLevel, TransistionDelay, false);

}

void ARGBMainMenuPlayerController::OpenGameplayLevel()
{
	UGameplayStatics::OpenLevel(this, GameplayLevelName);
}
