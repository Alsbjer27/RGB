// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGBColorComponent.h"
#include "RGBColorPlatform.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class RGB_API ARGBColorPlatform : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARGBColorPlatform();

	virtual void OnConstruction(const FTransform& Transform) override;
	URGBColorComponent* GetColorComponent() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Platform")
	TObjectPtr<UStaticMeshComponent> PlatformMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Platform")
	TObjectPtr<URGBColorComponent> ColorComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Materials")
	TObjectPtr<UMaterialInterface> RedMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Materials")
	TObjectPtr<UMaterialInterface> GreenMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Materials")
	TObjectPtr<UMaterialInterface> BlueMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Materials", meta = (ClampMin = "0"))
	int32 ColorMaterialIndex = 0;

private:
	UFUNCTION()
	void HandleColorChange(ERGBColor PreviousColor, ERGBColor NewColor);

	void ApplyColorMaterial(ERGBColor Color);
};
