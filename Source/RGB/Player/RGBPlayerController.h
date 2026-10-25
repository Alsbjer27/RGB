// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "../Progression/RGBAbilityUnlockSubsytem.h"
#include "RGBPlayerController.generated.h"


/**
 *
 */

class UInputMappingContext;
class UInputAction;

/** Which instruction text a newly unlocked ability shows. Weapons depend on how many were unlocked before. */
UENUM(BlueprintType)
enum class ERGBAbilityInstruction : uint8 {
	Dash,
	FirstWeapon,
	WeaponSwitching,
	AdditionalWeapon
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FRGBAbilityInstructionChangedSignature, bool, bVisible, ERGBAbilityInstruction, Instruction, ERGBPlayerAbility, Ability);

UCLASS()
class RGB_API ARGBPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ARGBPlayerController();

	UPROPERTY(BlueprintAssignable, Category = "RGB|Ability Instructions")
	FRGBAbilityInstructionChangedSignature OnAbilityInstructionChanged;

	bool GetActiveAbilityInstruction(ERGBAbilityInstruction& OutInstruction, ERGBPlayerAbility& OutAbility) const;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Transition", meta = (ClampMin = "0.0", Units = "s"))
	float InitialFadeInDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Transition", meta = (ClampMin = "0.0", Units = "s"))
	float InitialBlackDelay = 1.5f;

	virtual void SetupInputComponent() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Input")
	TObjectPtr<UInputAction> RestartAction;

	void RestartPlayer();

	void StartInitialFadeIn();
	void FinishInitialFadeIn();

	FTimerHandle InitialBlackDelayTimerHandle;
	FTimerHandle InitialFadeInTimerHandle;

	// Added above the gameplay context while an instruction is open. Its dismiss action must have Trigger When Paused enabled.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Ability Instructions")
	TObjectPtr<UInputMappingContext> AbilityInstructionMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Ability Instructions")
	TObjectPtr<UInputAction> DismissAbilityInstructionAction;

	// Prevents a jump press that coincides with the pickup from closing the instruction unread.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Ability Instructions", meta = (ClampMin = "0.0", Units = "s"))
	float MinimumAbilityInstructionTime = 0.6f;

private:
	struct FPendingAbilityInstruction
	{
		ERGBAbilityInstruction Instruction = ERGBAbilityInstruction::Dash;
		ERGBPlayerAbility Ability = ERGBPlayerAbility::Dash;
	};

	UFUNCTION()
	void HandleAbilityUnlocked(ERGBPlayerAbility Ability);

	void ShowNextAbilityInstruction();
	void DismissAbilityInstruction();
	void SetAbilityInstructionInputEnabled(bool bEnabled);

	TArray<FPendingAbilityInstruction> PendingAbilityInstructions;
	bool bAbilityInstructionVisible = false;
	bool bInitialFadeInFinished = false;
	double AbilityInstructionShownTime = 0.0;
};
