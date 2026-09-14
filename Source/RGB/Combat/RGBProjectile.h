// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Platforms/RGBColorComponent.h"
#include "RGBProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

class UMaterialInterface;

UCLASS()
class RGB_API ARGBProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARGBProjectile();

	void InitializeColor(ERGBColor InColor);

	ERGBColor GetProjectileColor() const { return ProjectileColor; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Projectile")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Material")
	TObjectPtr<UMaterialInterface> RedMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Material")
	TObjectPtr<UMaterialInterface> GreenMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Material")
	TObjectPtr<UMaterialInterface> BlueMaterial;
private:
	UFUNCTION()
	void HandleProjectileStopped(const FHitResult& ImpactResult);

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "RGB|Projectile")
	ERGBColor ProjectileColor = ERGBColor::Red;

	bool bColorInitialized = false;
};
