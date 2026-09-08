// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SpringArmComponent.h"
#include "RGBSideViewCameraComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = Camera, meta = (BlueprintSpawnableComponent))
class RGB_API URGBSideViewCameraComponent : public USpringArmComponent
{
	GENERATED_BODY()
public:
	URGBSideViewCameraComponent();
};
