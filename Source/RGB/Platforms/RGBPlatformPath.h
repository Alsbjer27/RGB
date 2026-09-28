// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGBPlatformPath.generated.h"

class USplineComponent;

UCLASS()
class RGB_API ARGBPlatformPath : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARGBPlatformPath();

	USplineComponent* GetPathSpline() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Platform Path")
	TObjectPtr<USplineComponent> PathSpline;

};
