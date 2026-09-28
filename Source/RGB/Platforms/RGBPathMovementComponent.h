// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RGBPathMovementComponent.generated.h"

class ARGBPlatformPath;



UCLASS( ClassGroup=(RGB), meta=(BlueprintSpawnableComponent) )
class RGB_API URGBPathMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	URGBPathMovementComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "RGB|Path Movement")
	void SetMovementEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "RGB|Path Movement")
	bool IsMovementEnabled() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Path Movement")
	TObjectPtr<ARGBPlatformPath> MovementPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Path Movement", meta = (ClampMin = "0.0"))
	float MovementSpeed = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Path Movement")
	bool bStartActive = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Path Movement")
	bool bFollowPathRotation = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Path Movement")
	bool bSnapToPathStart = true;

private:
	void ApplyPathTransform();

	float DistanceAlongPath = 0.0f;
	float MovementDirection = 1.0f;
	bool bMovementEnabled = false;
};
