// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGBDashAfterImage.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPoseableMeshComponent;
class USkeletalMeshComponent;

UCLASS()
class RGB_API ARGBDashAfterImage : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARGBDashAfterImage();

	virtual void Tick(float DeltaTime) override;

	void InitializeAfterImage(USkeletalMeshComponent* SourceMesh, UMaterialInterface* AfterImageMaterial, float Lifetime, const FLinearColor& Color, float InitialOpacity);


protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Dash AfterImage")
	TObjectPtr<UPoseableMeshComponent> PoseableMesh;


private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;

	float TotalLifetime = 0.2f;
	float ElapsedLifeTime = 0.0f;
	float StartingOpacity = 0.0f;
};
