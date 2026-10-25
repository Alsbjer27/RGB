// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGBJumpPad.generated.h"

class UStaticMeshComponent;
class USoundBase;

UCLASS()
class RGB_API ARGBJumpPad : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARGBJumpPad();

	float GetLaunchSpeed() const { return LaunchSpeed; }

	void PlayLaunchFeedback(const FVector& Loaction) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|JumpPad")
	TObjectPtr<UStaticMeshComponent> PadMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|JumpPad")
	float LaunchSpeed = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|JumpPad")
	TObjectPtr<USoundBase> LaunchSound;

};
