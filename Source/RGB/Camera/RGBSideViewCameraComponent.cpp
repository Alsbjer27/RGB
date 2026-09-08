// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBSideViewCameraComponent.h"

URGBSideViewCameraComponent::URGBSideViewCameraComponent()
{
	SetUsingAbsoluteRotation(true);
	SetRelativeRotation(FRotator(-10.0f, -90.0f, 0.0f));

	bUsePawnControlRotation = false;
	bInheritPitch = false;
	bInheritRoll = false;
	bInheritYaw = false;

	TargetArmLength = 2000.0f;
	TargetOffset = FVector(0.0f, 0.0f, 120.0f);

	bDoCollisionTest = false;

	// Start with direct following, change when we want static
	bEnableCameraLag = false;
	bEnableCameraRotationLag = false;
}
