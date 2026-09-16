// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "RGBPlayerCameraManager.generated.h"


UENUM(BlueprintType)
enum class ERGBCameraTestMode : uint8 {
	NormalPerspective,
	FlatArenaOnly,
	FlatBothViews
};

/**
 * 
 */
UCLASS()
class RGB_API ARGBPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	virtual void SetViewTarget(AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams = FViewTargetTransitionParams()) override;

protected:
	virtual void DoUpdateCamera(float DeltaTime) override;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Camera")
	ERGBCameraTestMode CameraTestMode = ERGBCameraTestMode::FlatArenaOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Camera")
	float FlatViewDistance = 80000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Camera")
	bool bUseLinearBlend = true;

private:
	bool ApplyDollyZoom(FMinimalViewInfo& View, float FlattenAmount) const;
	
	float GetTargetFlattenAmount(const AActor* Target) const;

	FMinimalViewInfo LastBaseView;

	bool bHasBaseView = false;

	float CurrentFlattenAmount = 0.0f;
	float BlendStartFlattenAmount = 0.0f;
};
