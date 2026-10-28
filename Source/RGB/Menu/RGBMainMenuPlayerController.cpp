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
#include "Engine/GameInstance.h"
#include "../Game/RGBGameSettingsSubsystem.h"

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

	if (IsValid(AdjustAction)) {
		EnhancedInput->BindAction(AdjustAction, ETriggerEvent::Triggered, this, &ARGBMainMenuPlayerController::HandleAdjust);
		EnhancedInput->BindAction(AdjustAction, ETriggerEvent::Completed, this, &ARGBMainMenuPlayerController::HandleAdjustReleased);
		EnhancedInput->BindAction(AdjustAction, ETriggerEvent::Canceled, this, &ARGBMainMenuPlayerController::HandleAdjustReleased);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("%s has no AdjustAction assigned; settings values cannot be changed."), *GetName());
	}

	if (IsValid(BackAction)) {
		EnhancedInput->BindAction(BackAction, ETriggerEvent::Started, this, &ARGBMainMenuPlayerController::HandleBack);
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("%s has no BackAction assigned; only Confirm can close menu pages."), *GetName());
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
		MenuDirector->OnCreditsReady.AddDynamic(this, &ARGBMainMenuPlayerController::HandleCreditsReady);
		MenuDirector->OnSettingsReady.AddDynamic(this, &ARGBMainMenuPlayerController::HandleSettingsReady);
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
		MenuDirector->OnCreditsReady.RemoveDynamic(this, &ARGBMainMenuPlayerController::HandleCreditsReady);
		MenuDirector->OnSettingsReady.RemoveDynamic(this, &ARGBMainMenuPlayerController::HandleSettingsReady);
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
	const float Direction = Value.Get<float>();

	// An analog stick rarely reports exactly zero when released, so "Completed" may never fire.
	// Re-arm navigation as soon as the input drops back under the threshold.
	if (FMath::Abs(Direction) < NavigationThreshold) {
		bNavigationHeld = false;
		return;
	}

	if (bSettingsOpen) {
		if (!bNavigationHeld) {
			bNavigationHeld = true;
			const int32 RowCount = 2;
			const int32 NewRow = FMath::Clamp(SettingsRow + (Direction > 0.0f ? -1 : 1), 0, RowCount - 1);
			if (NewRow != SettingsRow) {
				SettingsRow = NewRow;
				OnSettingsRowChanged.Broadcast(SettingsRow);
			}
		}
		return;
	}

	if (bCreditsOpen) {
		return;
	}

	if (bLevelTransitionActive) {
		return;
	}

	if (MenuDirector.IsValid() &&
		MenuDirector->IsSequenceRunning())
	{
		return;
	}

	if (bNavigationHeld) {
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
	if (bCreditsOpen) {
		SetCreditsOpen(false);
		return;
	}

	if (bSettingsOpen) {
		// Values change with left/right; Back closes the page.
		return;
	}

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

void ARGBMainMenuPlayerController::HandleBack(const FInputActionValue& Value)
{
	if (bCreditsOpen) {
		SetCreditsOpen(false);
	}
	else if (bSettingsOpen) {
		SetSettingsOpen(false);
	}
}

void ARGBMainMenuPlayerController::HandleAdjust(const FInputActionValue& Value)
{
	const float Direction = Value.Get<float>();
	if (FMath::Abs(Direction) < NavigationThreshold) {
		bAdjustHeld = false;
		return;
	}
	if (!bSettingsOpen || bAdjustHeld) {
		return;
	}
	bAdjustHeld = true;
	AdjustSelectedSetting(Direction > 0.0f ? 1 : -1);
}

void ARGBMainMenuPlayerController::HandleAdjustReleased(const FInputActionValue& Value)
{
	bAdjustHeld = false;
}

void ARGBMainMenuPlayerController::HandleSettingsReady()
{
	SetSettingsOpen(true);
}

void ARGBMainMenuPlayerController::SetSettingsOpen(bool bOpen)
{
	if (bSettingsOpen == bOpen) {
		return;
	}
	bSettingsOpen = bOpen;
	bNavigationHeld = false;
	bAdjustHeld = false;
	if (bSettingsOpen) {
		SettingsRow = 0;
		OnSettingsRowChanged.Broadcast(SettingsRow);
	}
	OnSettingsVisibilityChanged.Broadcast(bSettingsOpen);
}

void ARGBMainMenuPlayerController::AdjustSelectedSetting(int32 Step)
{
	const UGameInstance* GameInstance = GetGameInstance();
	URGBGameSettingsSubsystem* Settings = GameInstance ? GameInstance->GetSubsystem<URGBGameSettingsSubsystem>() : nullptr;
	if (!IsValid(Settings)) {
		return;
	}

	if (SettingsRow == 0) {
		const int32 Current = static_cast<int32>(Settings->GetDifficulty());
		const int32 Next = FMath::Clamp(Current + Step, 0, static_cast<int32>(ERGBDifficulty::Hard));
		Settings->SetDifficulty(static_cast<ERGBDifficulty>(Next));
	}
	else {
		// Steps of 10 %, kept as whole percentages to avoid float drift.
		const int32 CurrentPercent = FMath::RoundToInt(Settings->GetMasterVolume() * 10.0f) * 10;
		const int32 NextPercent = FMath::Clamp(CurrentPercent + Step * 10, 0, 100);
		Settings->SetMasterVolume(NextPercent / 100.0f);
	}
}

void ARGBMainMenuPlayerController::HandleCreditsReady()
{
	SetCreditsOpen(true);
}

void ARGBMainMenuPlayerController::SetCreditsOpen(bool bOpen)
{
	if (bCreditsOpen == bOpen) {
		return;
	}
	bCreditsOpen = bOpen;
	bNavigationHeld = false;
	OnCreditsVisibilityChanged.Broadcast(bCreditsOpen);
}
