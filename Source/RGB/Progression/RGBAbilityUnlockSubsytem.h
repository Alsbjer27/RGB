// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RGBAbilityUnlockSubsytem.generated.h"

/**
 * 
 */
UENUM(BlueprintType)
enum class ERGBPlayerAbility : uint8 {
	Dash,
	RedWeapon,
	GreenWeapon,
	BlueWeapon
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRGBAbilityUnlockedSignature, ERGBPlayerAbility, Ability);

UCLASS()
class RGB_API URGBAbilityUnlockSubsytem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "RGB|Progression")
	bool IsAbilityUnlocked(ERGBPlayerAbility Ability) const;
	bool UnlockAbility(ERGBPlayerAbility Ability);

	UPROPERTY(BlueprintAssignable, Category = "RGB|Progression")
	FRGBAbilityUnlockedSignature OnAbilityUnlocked;

private:
	UPROPERTY(Transient)
	TSet<ERGBPlayerAbility> UnlockedAbilities;


	
};
