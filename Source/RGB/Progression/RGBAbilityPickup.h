// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGBAbilityUnlockSubsytem.h"
#include "RGBAbilityPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;
class ARGBPlayerCharacter;

UCLASS()
class RGB_API ARGBAbilityPickup : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARGBAbilityPickup();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Pickup")
	TObjectPtr<USphereComponent> PickupCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Pickup")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Pickup")
	ERGBPlayerAbility GrantedAbility = ERGBPlayerAbility::Dash;

private:
	UFUNCTION()
	void HandlePickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
