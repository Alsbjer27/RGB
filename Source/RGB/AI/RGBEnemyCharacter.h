// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RGBEnemyAITypes.h"
#include "RGBArenaEnemyTuning.h"
#include "../Platforms/RGBColorComponent.h"
#include "GameFramework/Character.h"
#include "RGBEnemyCharacter.generated.h"

class UAnimMontage;
class UMaterialInterface;
class ARGBEnemyCharacter;
class ARGBArenaCameraZone;
class URGBArenaEnemyCoordinatorComponent;
class ARGBArenaControl;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRGBEnemyEliminatedSignature, ARGBEnemyCharacter*, EliminatedEnemy);

UCLASS()
class RGB_API ARGBEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARGBEnemyCharacter();

	virtual void Tick(float DeltaTime) override;
	virtual ERGBEnemyMobilityType GetEnemyMobilityType() const { return ERGBEnemyMobilityType::Ground; }

	bool RecieveColorHit(ERGBColor ProjectileColor);
	void SetEnemyCoordinator(URGBArenaEnemyCoordinatorComponent* NewCoordinator);
	void SetArenaCameraZone(ARGBArenaCameraZone* NewArenaCameraZone);
	void SetArenaAIActive(bool bNewActive);
	void SetArenaRespawnManaged(bool bManaged);
	void PrepareForArenaRespawn();
	void RestoreForArenaRespawn(const FTransform& SpawnTransform);
	void SetArenaMovementSpeedMultiplier(float NewMultiplier);
	float GetArenaMovementSpeedMultiplier() const { return ArenaMovementSpeedMultiplier; }
	void SetOwningArena(ARGBArenaControl* NewArena);
	ARGBArenaControl* GetOwningArena() const { return OwningArena.Get(); }
	void ApplyArenaAttributes(const FRGBEnemyArenaAttributes& NewAttributes);
	bool IsAvailableForArenaAI() const { return bArenaAIActive && !bEliminated && !IsHidden() && !IsActorBeingDestroyed(); }
	virtual bool CanBeAttackCandidate(const ACharacter* Player) const;
	virtual bool IsReadyToAttack(const ACharacter* Player) const;
	float GetChasePreference() const { return ArenaAttributes.bEnabled ? ArenaAttributes.ChasePreference : 1.0f; }

	UFUNCTION(BlueprintPure, Category = "RGB|AI")
	ERGBEnemyActionState GetActionState() const { return ActionState; }

	UPROPERTY(BlueprintAssignable, Category = "RGB|Enemy")
	FRGBEnemyEliminatedSignature OnEnemyEliminated;

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

	UPROPERTY(EditAnywhere, Category = "RGB|Attack", meta = (ClampMin = "0.01", Units = "s"))
	float AttackDuration = 0.6f;

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

	virtual bool IsPlayerWithinAttackRange(const ACharacter* Player) const;
	virtual void HandleArenaAIActiveChanged(bool bNewActive);
	virtual void HandleArenaMovementSpeedMultiplierChanged(float NewMultiplier);
	virtual void HandleArenaAttributesChanged();
	bool IsPlayerVisibleForChase(const ACharacter* Player) const;
	bool CanStartAttackNow() const;
	const FRGBEnemyArenaAttributes& GetArenaAttributes() const { return ArenaAttributes; }
	void StartAttackAgainst(ACharacter* Player);
	bool GetCoordinationAssignment(FRGBEnemyCoordinationAssignment& OutAssignment) const;
	void SetActionState(ERGBEnemyActionState NewState);
	bool TryAcquireCoordinatedAttackPermission();
	void ReleaseCoordinatedAttackPermission();
	bool HasEnemyCoordinator() const;
	bool IsArenaAIActive() const { return bArenaAIActive; }
	ARGBArenaCameraZone* GetArenaCameraZone() const { return ArenaCameraZone.Get(); }
	bool IsCoordinationTargetReservedByOther(const AActor* Target) const;
	bool TryReserveCoordinationTarget(AActor* Target);
	void ReleaseCoordinationTarget(const AActor* Target = nullptr);
	bool IsAttackInProgress() const { return bAttacking; }
	virtual void ApplyColorMaterial(ERGBColor Color);

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "RGB|AI|Arena Attributes")
	FRGBEnemyArenaAttributes ArenaAttributes;


private:
	friend class FRGBInactiveEnemyDamageTest;
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
	void FinishAttack();
	bool UpdateCoordinatedMovement(float DeltaTime);

	void TryApplyAttackDamage();
	FTimerHandle AttackDamageTimer;
	FTimerHandle AttackFinishTimer;

	bool bAttacking = false;
	double NextAttackAllowedTime = 0.0;
	bool bArenaAIActive = true;
	bool bArenaRespawnManaged = false;
	float ArenaMovementSpeedMultiplier = 1.0f;
	float BaseGroundMovementSpeed = 200.0f;
	bool bHasCapturedGroundMovementSpeed = false;
	double NextPatrolDirectionTime = 0.0;
	float CurrentAttackCooldown = 1.0f;
	void UpdateGroundMovementSpeed(bool bChasing);

	UPROPERTY(Transient)
	TObjectPtr<ARGBArenaControl> OwningArena;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "RGB|AI")
	ERGBEnemyActionState ActionState = ERGBEnemyActionState::Idle;

	UPROPERTY(Transient)
	TObjectPtr<URGBArenaEnemyCoordinatorComponent> EnemyCoordinator;

	UPROPERTY(Transient)
	TObjectPtr<ARGBArenaCameraZone> ArenaCameraZone;

	UPROPERTY(Transient)
	TWeakObjectPtr<ACharacter> TargetPlayer;

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "RGB|Health")
	int32 CurrentHealth = 1;

	bool bEliminated = false;

	void TransferColorSupportingPlatform();
};
