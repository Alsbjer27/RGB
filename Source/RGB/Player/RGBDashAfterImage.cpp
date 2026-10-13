// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBDashAfterImage.h"

#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

// Sets default values
ARGBDashAfterImage::ARGBDashAfterImage()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	PoseableMesh = CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("PoseableMesh"));
	SetRootComponent(PoseableMesh);
	PoseableMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PoseableMesh->SetGenerateOverlapEvents(false);
	PoseableMesh->SetCastShadow(false);
	PoseableMesh->SetReceivesDecals(false);
}

void ARGBDashAfterImage::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ElapsedLifeTime += DeltaTime;

	const float LifetimeAlpha = FMath::Clamp(ElapsedLifeTime / TotalLifetime, 0.0f, 1.0f);

	if (IsValid(DynamicMaterial)) {
		DynamicMaterial->SetScalarParameterValue(TEXT("AfterImageOpaciy"), StartingOpacity * (1.0f - LifetimeAlpha));
	}

	if (LifetimeAlpha >= 1.0f) {
		Destroy();
	}
}

void ARGBDashAfterImage::InitializeAfterImage(USkeletalMeshComponent* SourceMesh, UMaterialInterface* AfterImageMaterial, float Lifetime, const FLinearColor& Color, float InitialOpacity)
{
	if (!IsValid(SourceMesh) || !IsValid(AfterImageMaterial) || !IsValid(PoseableMesh)) {
		Destroy();
		return;
	}

	TotalLifetime = FMath::Max(Lifetime, 0.01f);
	ElapsedLifeTime = 0.0f;
	StartingOpacity = FMath::Clamp(InitialOpacity, 0.0f, 1.0f);

	SetActorTransform(SourceMesh->GetComponentTransform());

	PoseableMesh->SetSkinnedAssetAndUpdate(SourceMesh->GetSkinnedAsset());
	PoseableMesh->CopyPoseFromSkeletalComponent(SourceMesh);

	DynamicMaterial = UMaterialInstanceDynamic::Create(AfterImageMaterial, this);

	if (!IsValid(DynamicMaterial)) {
		Destroy();
		return;
	}

	DynamicMaterial->SetVectorParameterValue(
		TEXT("AfterimageColor"),
		Color);

	DynamicMaterial->SetScalarParameterValue(
		TEXT("AfterimageOpacity"),
		StartingOpacity);

	const int32 MaterialCount = FMath::Max(PoseableMesh->GetNumMaterials(), 1);

	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex) {
		PoseableMesh->SetMaterial(MaterialIndex, DynamicMaterial);
	}
	SetActorTickEnabled(true);
}