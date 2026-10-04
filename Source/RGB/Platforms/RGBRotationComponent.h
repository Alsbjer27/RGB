// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RGBRotationComponent.generated.h"


UENUM(BlueprintType)
enum class ERGBRotationMode : uint8 {
	Continuous,
	Stepped
};

UCLASS( ClassGroup=(RGB), meta=(BlueprintSpawnableComponent) )
class RGB_API URGBRotationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	URGBRotationComponent();

	virtual void TickComponent(float DetlaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "RGB|Rotation")
	void SetRotationEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "RGB|Rotation")
	bool IsRotationEnabled() const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Rotation")
	ERGBRotationMode RotationMode = ERGBRotationMode::Continuous;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Rotation")
	FVector RotationAxis = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Rotation", meta = (ClampMin = "0.0"))
	float RotationSpeed = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Rotation", meta = (EditDefaultsOnly = "RotationMode == ERGBRotationMode::Stepped"))
	float StepAngle = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Rotation", meta = (ClampMin = "0.0", EditCondition = "RotationMode == ERGBRotationMode::Stepped"))
	float DelayBetweenSteps = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Rotation")
	bool bStartActive = false;

private:
	void RotateByDegrees(float Degrees);

	float RemainingStepDegrees = 0.0f;
	float RemainingDelay = 0.0f;
	bool bRotationEnabled = false;
};
