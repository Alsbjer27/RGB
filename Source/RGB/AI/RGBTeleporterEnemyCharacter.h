// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RGBEnemyCharacter.h"
#include "RGBTeleporterEnemyCharacter.generated.h"

/**
 * 
 */
class ATargetPoint;

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

private:
	void FindTeleportPoints();
	void TryTeleport();


	FTimerHandle TeleportTimer;

	TArray<TWeakObjectPtr<ATargetPoint>> TeleportPoints;
	TWeakObjectPtr<ATargetPoint> LastTeleportPoint;

	bool IsTeleportDestinationClear(const FVector& Destination) const;
	
};
