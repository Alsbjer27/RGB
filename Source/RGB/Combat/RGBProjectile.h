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

class USoundBase;

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

	UFUNCTION(BlueprintImplementableEvent, Category = "RGB|Projectile|VFX")
	void OnProjectileColorInitialized(FLinearColor NewColor);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Projectile|VFX")
	FLinearColor RedVFXColor = FLinearColor(1.0f, 0.02f, 0.01f, 1.0f);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Projectile|VFX")
	FLinearColor GreenVFXColor = FLinearColor(0.02f, 1.0f, 0.05f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Projectile|VFX")
	FLinearColor BlueVFXColor = FLinearColor(0.02f, 0.02f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Projectile|Audio")
	TObjectPtr<USoundBase> ImpactSound;

private:
	UFUNCTION()
	void HandleProjectileStopped(const FHitResult& ImpactResult);

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "RGB|Projectile")
	ERGBColor ProjectileColor = ERGBColor::Red;

	bool bColorInitialized = false;
};
