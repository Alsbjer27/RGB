// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBAbilityUnlockSubsytem.h"

bool URGBAbilityUnlockSubsytem::IsAbilityUnlocked(ERGBPlayerAbility Ability) const
{
	return UnlockedAbilities.Contains(Ability);
}

bool URGBAbilityUnlockSubsytem::UnlockAbility(ERGBPlayerAbility Ability)
{
	if (UnlockedAbilities.Contains(Ability)) {
		return false;
	}

	UnlockedAbilities.Add(Ability);
	OnAbilityUnlocked.Broadcast(Ability);
	return true;
}
