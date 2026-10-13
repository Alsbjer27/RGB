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

	virtual void Tick(float DeltaTime) override;
	virtual ERGBEnemyMobilityType GetEnemyMobilityType() const override { return ERGBEnemyMobilityType::Teleporter; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandleArenaAIActiveChanged(bool bNewActive) override;

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport|Timing", meta = (ClampMin = "0.1", Units = "s"))
	float MinimumTeleportIdleTime = 1.5f;

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport|Timing", meta = (ClampMin = "0.1", Units = "s"))
	float MaximumTeleportIdleTime = 6.0f;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Teleport|Effects")
	TObjectPtr<UNiagaraComponent> TeleportChargeEffect;

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport|Effects")
	FName TeleportChargeParameterName = TEXT("User.TeleportCharge");

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport|Effects", meta = (ClampMin = "0.1"))
	float TeleportChargeExponent = 2.0f;

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport|Effects", meta = (ClampMin = "0.0", ClampMax = "0.99"))
	float ImplosionStartCharge = 0.6f;

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport|Effects", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float ImplosionMinimumScale = 0.08f;

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport|Effects", meta = (ClampMin = "0.1"))
	float ImplosionExponent = 2.0f;

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport|Decision", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PlayerProximityWeight = 0.65f;

	UPROPERTY(EditAnywhere, Category = "RGB|Teleport|Effects", meta = (ClampMin = "0.0", Units = "s"))
	float TeleportEffectDelay = 1.25f;

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

	void ScheduleNextTeleport();
	bool IsTeleportPointReservedByOther(const ATargetPoint* Point) const;

	void BeginTeleport(const FVector& Destination, const FRotator& DestinationRotation, ATargetPoint* DestiantionPoint);
	void CompleteTeleport();
	void SpawnTeleportEffect(UNiagaraSystem* Effect, const FVector& Location) const;

	void UpdateTeleportCharge(float DeltaTime);
	float TeleportChargeElapsed = 0.0f;
	FVector DefaultMeshRelativeScale = FVector::OneVector;
	bool bHasCapturedDefaultMeshScale = false;

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
