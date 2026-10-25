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

#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformTime.h"
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
	else
	{
		bInitialFadeInFinished = true;
	}

	if (!IsLocalController()) {
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance()) {
		if (URGBAbilityUnlockSubsytem* UnlockSubsystem = GameInstance->GetSubsystem<URGBAbilityUnlockSubsytem>()) {
			UnlockSubsystem->OnAbilityUnlocked.AddUniqueDynamic(this, &ARGBPlayerController::HandleAbilityUnlocked);
		}
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

	if (UGameInstance* GameInstance = GetGameInstance()) {
		if (URGBAbilityUnlockSubsytem* UnlockSubsystem = GameInstance->GetSubsystem<URGBAbilityUnlockSubsytem>()) {
			UnlockSubsystem->OnAbilityUnlocked.RemoveDynamic(this, &ARGBPlayerController::HandleAbilityUnlocked);
		}
	}

	if (bAbilityInstructionVisible) {
		SetAbilityInstructionInputEnabled(false);
		bAbilityInstructionVisible = false;
	}
	PendingAbilityInstructions.Reset();

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

	if (DismissAbilityInstructionAction)
	{
		// Dismiss on release so the held key is up before the gameplay jump mapping becomes active again.
		EnhancedInput->BindAction(DismissAbilityInstructionAction, ETriggerEvent::Completed, this, &ARGBPlayerController::DismissAbilityInstruction);
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

	// Pickups collected during the level-entry fade wait until the screen is visible.
	bInitialFadeInFinished = true;
	ShowNextAbilityInstruction();
}

bool ARGBPlayerController::GetActiveAbilityInstruction(ERGBAbilityInstruction& OutInstruction, ERGBPlayerAbility& OutAbility) const
{
	if (!bAbilityInstructionVisible || PendingAbilityInstructions.IsEmpty()) {
		return false;
	}

	OutInstruction = PendingAbilityInstructions[0].Instruction;
	OutAbility = PendingAbilityInstructions[0].Ability;
	return true;
}

void ARGBPlayerController::HandleAbilityUnlocked(ERGBPlayerAbility Ability)
{
	FPendingAbilityInstruction NewInstruction;
	NewInstruction.Ability = Ability;

	if (Ability != ERGBPlayerAbility::Dash) {
		// Count at unlock time, so queued weapon pickups keep their own first/second/third wording.
		const UGameInstance* GameInstance = GetGameInstance();
		const URGBAbilityUnlockSubsytem* UnlockSubsystem = GameInstance ? GameInstance->GetSubsystem<URGBAbilityUnlockSubsytem>() : nullptr;
		const ERGBPlayerAbility Weapons[] = { ERGBPlayerAbility::RedWeapon, ERGBPlayerAbility::GreenWeapon, ERGBPlayerAbility::BlueWeapon };
		int32 UnlockedWeaponCount = 0;

		for (const ERGBPlayerAbility Weapon : Weapons) {
			if (IsValid(UnlockSubsystem) && UnlockSubsystem->IsAbilityUnlocked(Weapon)) {
				++UnlockedWeaponCount;
			}
		}

		NewInstruction.Instruction = UnlockedWeaponCount <= 1 ? ERGBAbilityInstruction::FirstWeapon
			: UnlockedWeaponCount == 2 ? ERGBAbilityInstruction::WeaponSwitching
			: ERGBAbilityInstruction::AdditionalWeapon;
	}

	PendingAbilityInstructions.Add(NewInstruction);
	ShowNextAbilityInstruction();
}

void ARGBPlayerController::ShowNextAbilityInstruction()
{
	if (bAbilityInstructionVisible || !bInitialFadeInFinished || PendingAbilityInstructions.IsEmpty()) {
		return;
	}

	if (!AbilityInstructionMappingContext || !DismissAbilityInstructionAction) {
		// Never pause without a way to close the instruction.
		UE_LOG(LogTemp, Warning, TEXT("%s cannot show ability instructions: assign Ability Instruction Mapping Context and Dismiss Ability Instruction Action."), *GetName());
		PendingAbilityInstructions.Reset();
		return;
	}

	bAbilityInstructionVisible = true;
	AbilityInstructionShownTime = FPlatformTime::Seconds();
	SetAbilityInstructionInputEnabled(true);
	UGameplayStatics::SetGamePaused(this, true);

	const FPendingAbilityInstruction& Current = PendingAbilityInstructions[0];
	OnAbilityInstructionChanged.Broadcast(true, Current.Instruction, Current.Ability);
}

void ARGBPlayerController::DismissAbilityInstruction()
{
	if (!bAbilityInstructionVisible || FPlatformTime::Seconds() - AbilityInstructionShownTime < MinimumAbilityInstructionTime) {
		return;
	}

	const FPendingAbilityInstruction Dismissed = PendingAbilityInstructions[0];
	PendingAbilityInstructions.RemoveAt(0);
	bAbilityInstructionVisible = false;
	SetAbilityInstructionInputEnabled(false);
	OnAbilityInstructionChanged.Broadcast(false, Dismissed.Instruction, Dismissed.Ability);

	if (PendingAbilityInstructions.IsEmpty()) {
		UGameplayStatics::SetGamePaused(this, false);
		return;
	}

	ShowNextAbilityInstruction();
}

void ARGBPlayerController::SetAbilityInstructionInputEnabled(bool bEnabled)
{
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer) : nullptr;

	if (!InputSubsystem || !AbilityInstructionMappingContext) {
		return;
	}

	if (bEnabled) {
		InputSubsystem->AddMappingContext(AbilityInstructionMappingContext, 10);
	}
	else {
		InputSubsystem->RemoveMappingContext(AbilityInstructionMappingContext);
	}
}