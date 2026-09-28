// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBColorGroupActivator.h"

#include "RGBColorPlatform.h"
#include "RGBPathMovementComponent.h"
#include "TimerManager.h"

// Sets default values
ARGBColorGroupActivator::ARGBColorGroupActivator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void ARGBColorGroupActivator::BeginPlay()
{
	Super::BeginPlay();

	for (ARGBColorPlatform* Platform : WatchedPlatforms) {
		if (!IsValid(Platform)) {
			continue;
		}

		if (URGBColorComponent* Color = Platform->GetColorComponent()) {
			Color->OnColorChanged.AddUniqueDynamic(this, &ARGBColorGroupActivator::HandlePlatformColorChanged);
		}
	}

	GetWorldTimerManager().SetTimerForNextTick(this, &ARGBColorGroupActivator::EvaluateColorCondition);
}

void ARGBColorGroupActivator::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (ARGBColorPlatform* Platform : WatchedPlatforms) {
		if (!IsValid(Platform)) {
			continue;
		}

		if (URGBColorComponent* Color = Platform->GetColorComponent()) {
			Color->OnColorChanged.RemoveDynamic(this, &ARGBColorGroupActivator::HandlePlatformColorChanged);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ARGBColorGroupActivator::HandlePlatformColorChanged(ERGBColor PreviousColor, ERGBColor NewColor)
{
	EvaluateColorCondition();
}

void ARGBColorGroupActivator::EvaluateColorCondition()
{
	if (WatchedPlatforms.Num() < 2) {
		SetControlledMovementEnabled(false);
		UE_LOG(LogTemp, Warning, TEXT("%s needs at least two watched color platforms."), *GetName());
		return;
	}

	ARGBColorPlatform* FirstPlatform = WatchedPlatforms[0];

	if (!IsValid(FirstPlatform) || !IsValid(FirstPlatform->GetColorComponent())) {
		SetControlledMovementEnabled(false);
		return;
	}

	const ERGBColor TargetColor = bRequireSpecificColor ? RequiredColor : FirstPlatform->GetColorComponent()->GetCurrentColor();

	bool bAllColorsMatch = true;

	for (ARGBColorPlatform* Platform : WatchedPlatforms) {
		if (!IsValid(Platform)) {
			bAllColorsMatch = false;
			break;
		}

		URGBColorComponent* Color = Platform->GetColorComponent();

		if (!IsValid(Color) || Color->GetCurrentColor() != TargetColor) {
			bAllColorsMatch = false;
			break;
		}
	}

	if (bAllColorsMatch) {
		SetControlledMovementEnabled(true);
	}
	else if(bDisableWhenColorDiffer) {
		SetControlledMovementEnabled(false);
	}
}

void ARGBColorGroupActivator::SetControlledMovementEnabled(bool bEnabled)
{
	for (AActor* ControlledActor : ControlledActors)
	{
		if (!IsValid(ControlledActor)) {
			continue;
		}

		URGBPathMovementComponent* Movement = ControlledActor->FindComponentByClass< URGBPathMovementComponent>();

		if (IsValid(Movement)) {
			Movement->SetMovementEnabled(bEnabled);
		} 
		else {
			UE_LOG(LogTemp, Warning, TEXT("%s controls %s, but it has no RGB Path Movement Component."), *GetName(), *GetNameSafe(ControlledActor));
		}
	}
}
