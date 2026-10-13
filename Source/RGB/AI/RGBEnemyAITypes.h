#pragma once

#include "CoreMinimal.h"
#include "RGBEnemyAITypes.generated.h"

UENUM(BlueprintType)
enum class ERGBEnemyActionState : uint8
{
	Idle,
	Patrol,
	Chase,
	Turning,
	AttackWindup,
	AttackCooldown,
	TeleportCharge,
	AoEWindup,
	Disabled
};

UENUM(BlueprintType)
enum class ERGBEnemyCoordinationRole : uint8
{
	Unassigned,
	MeleeAttacker,
	Pressure,
	Special
};

UENUM(BlueprintType)
enum class ERGBEnemyMobilityType : uint8
{
	Ground,
	Flying,
	Teleporter
};

USTRUCT(BlueprintType)
struct RGB_API FRGBEnemyCoordinationAssignment
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|AI")
	ERGBEnemyCoordinationRole Role = ERGBEnemyCoordinationRole::Unassigned;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|AI")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|AI")
	bool bHasTargetLocation = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|AI")
	bool bCanAttack = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|AI")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|AI")
	double LeaseExpiresAt = 0.0;
};
