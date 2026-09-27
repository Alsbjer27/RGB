// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "../Platforms/RGBColorComponent.h"
#include "GameFramework/Character.h"
#include "RGBEnemyCharacter.generated.h"

class UAnimMontage;
class UMaterialInterface;

UCLASS()
class RGB_API ARGBEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARGBEnemyCharacter();

	virtual void Tick(float DeltaTime) override;

	bool RecieveColorHit(ERGBColor ProjectileColor);

	void DespawnForArenaCompletion();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category = "RGB|Patrol")
	bool bPatrolEnabled = true;

	UPROPERTY(EditDefaultsOnly, Category = "RGB|Patrol")
	float EdgeLookAhead = 30.0f;

	UPROPERTY(EditDefaultsOnly, Category = "RGB|Patrol")
	float MaxPatrolDrop = 30.0f;

	UPROPERTY(EditDefaultsOnly, Category = "RGB|Detection")
	float DetectionRange = 800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "RGB|Detection")
	float LoseInterestRange = 1000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "RGB|Detection")
	float DetectionHeightTolerance = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "RGB|Detection")
	float StopDistanceFromPlayer = 40.0f;

	UPROPERTY(EditAnywhere, Category = "RGB|Animation")
	TObjectPtr<UAnimMontage> TurnMontage;

	UPROPERTY(EditAnywhere, Category = "RGB|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditAnywhere, Category = "RGB|Attack")
	float AttackCooldown = 1.0f;

	UPROPERTY(EditAnywhere, Category = "RGB|Attack")
	float AttackPlayRate = 1.25f;

	UPROPERTY(EditAnywhere, Category = "RGB|Attack")
	int32 AttackDamage = 1;

	UPROPERTY(EditAnywhere, Category = "RGB|Attack")
	float AttackHitDelay = 0.35f;

	UPROPERTY(EditAnywhere, Category = "RGB|Attack")
	float AttackHitRangePadding = 20.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Color")
	TObjectPtr<URGBColorComponent> ColorComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Color Materials")
	TObjectPtr<UMaterialInterface> RedBaseMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Color Materials")
	TObjectPtr<UMaterialInterface> RedEmissionMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Color Materials")
	TObjectPtr<UMaterialInterface> GreenBaseMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Color Materials")
	TObjectPtr<UMaterialInterface> GreenEmissionMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Color Materials")
	TObjectPtr<UMaterialInterface> BlueBaseMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Color Materials")
	TObjectPtr<UMaterialInterface> BlueEmissionMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Health")
	int32 MaxHealth = 1;

	bool IsPlayerWithinAttackRange(const ACharacter* Player) const;
	void StartAttackAgainst(ACharacter* Player);


private:
	bool CanWalkInDirection(float Direction, float DeltaTime) const;
	void UpdatePlayerDetection();

	float PatrolDirection = 1.0f;
	FTimerHandle DetectionTimer;

	void StartTurn(float NewDirection);
	void UpdateTurn(float DeltaTime);

	bool bTurning = false;
	float TurnDuration = 0.0f;
	float TurnStartYaw = 0.0f;

	void StartAttack();
	void UpdateAttack();

	void TryApplyAttackDamage();
	FTimerHandle AttackDamageTimer;

	bool bAttacking = false;
	double NextAttackAllowedTime = 0.0;

	UPROPERTY(Transient)
	TWeakObjectPtr<ACharacter> TargetPlayer;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "RGB|Health")
	int32 CurrentHealth = 1;

	bool bEliminated = false;

	void ApplyColorMaterial(ERGBColor Color);

	void TransferColorSupportingPlatform();
};
