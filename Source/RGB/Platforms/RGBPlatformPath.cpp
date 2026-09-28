// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBPlatformPath.h"
#include "Components/SplineComponent.h"


// Sets default values
ARGBPlatformPath::ARGBPlatformPath()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	PathSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PathSpline"));
	SetRootComponent(PathSpline);

	PathSpline->SetClosedLoop(false);
}

USplineComponent* ARGBPlatformPath::GetPathSpline() const
{
	return PathSpline;
}
