// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RGBEnemyCharacter.h"
#include "RGBTeleporterEnemyCharacter.generated.h"

/**
 * 
 */
class ATargetPoint;
class ACharacter;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class RGB_API ARGBTeleporterEnemyCharacter : public ARGBEnemyCharacter
{
	GENERATED_BODY()
public:

	ARGBTeleporterEnemyCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport")
	float TeleportInterval = 2.0f;

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport")
	float TeleportPointSearchRadius = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport")
	float MinimumTeleportDistance = 100.0f;

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport")
	FName TeleportPointTag = TEXT("TeleporterPoint");

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport")
	float TeleportOccupancyPadding = 20.0f;

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport|Effects")
	TObjectPtr<UNiagaraSystem> TeleportDepartureEffect;

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport|Effects")
	TObjectPtr<UNiagaraSystem> TeleportArrivalEffect;

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport|Effects", meta = (ClampMin = "0.0", Units = "s"))
	float TeleportEffectDelay = 0.15f;

	UPROPERTY(EditAnywhere, Category = "RGB|AoE", meta = (ClampMin = "0.0"))
	float AoERadius = 250.0f;

	UPROPERTY(EditAnywhere, Category = "RGB|AoE", meta = (ClampMin = "0.0"))
	int32 AoEDamage = 1;

	UPROPERTY(EditAnywhere, Category = "RGB|AoE", meta = (ClampMin = "0.0", Units = "s"))
	float AoEWindupDuration = 0.75f;

	UPROPERTY(EditAnywhere, Category = "RGB|AoE", meta = (ClampMin = "0.0", Units = "s"))
	float AoECooldown = 2.0f;

	UPROPERTY(EditAnywhere, Category = "RGB|AoE", meta = (ClampMin = "0.01", Units = "s"))
	float AoECheckInterval = 0.1f;

	UPROPERTY(EditAnywhere, Category = "RGB|AoE")
	FVector AoEWarningOffset = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|AoE")
	TObjectPtr<UNiagaraComponent> AoEWarningEffect;

private:
	void FindTeleportPoints();
	void TryTeleport();

	void BeginTeleport(const FVector& Destination, const FRotator& DestinationRotation, ATargetPoint* DestiantionPoint);
	void CompleteTeleport();
	void SpawnTeleportEffect(UNiagaraSystem* Effect, const FVector& Location) const;

	void TryStartAoEAttack();
	void ResolveAoEAttack();
	bool IsPlayerWithinAoERadius(const ACharacter* Player) const;

	void SetAoEWarningActive(bool bActive);

	FTimerHandle TeleportTimer;
	FTimerHandle AoECheckTimer;
	FTimerHandle AoEWindupTimer;
	FTimerHandle TeleportTransistionTimer;

	TArray<TWeakObjectPtr<ATargetPoint>> TeleportPoints;
	TWeakObjectPtr<ATargetPoint> LastTeleportPoint;

	TWeakObjectPtr<ATargetPoint> PendingTeleportPoint;
	FVector PendingTeleportDestiantion = FVector::ZeroVector;
	FRotator PendingTeleportRotation = FRotator::ZeroRotator;
	bool bTeleportInProgress = false;

	bool bAoEWindInProgress = false;
	double NextAoEAttackAllowedTime = 0.0;

	bool IsTeleportDestinationClear(const FVector& Destination) const;
};
