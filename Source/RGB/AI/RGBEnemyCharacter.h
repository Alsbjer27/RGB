// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "RGBEnemyCharacter.generated.h"

class UAnimSequence;

UCLASS()
class RGB_API ARGBEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARGBEnemyCharacter();

	virtual void Tick(float DeltaTime) override;

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
	TObjectPtr<UAnimSequence> WalkAnimation;

	UPROPERTY(EditAnywhere, Category = "RGB|Animation")
	TObjectPtr<UAnimSequence> TurnAnimation;

private:
	bool CanWalkInDirection(float Direction, float DeltaTime) const;
	void UpdatePlayerDetection();

	float PatrolDirection = 1.0f;
	FTimerHandle DetectionTimer;

	void StartTurn(float NewDirection);
	void UpdateTurn(float DeltaTime);
	void ResumeWalkAnimation();

	bool bTurning = false;
	float TurnElapsed = 0.0f;
	float TurnDuration = 0.0f;
	float TurnStartYaw = 0.0f;

	UPROPERTY(Transient)
	TWeakObjectPtr<ACharacter> TargetPlayer;
};
