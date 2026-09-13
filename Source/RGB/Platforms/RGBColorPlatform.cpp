// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBColorPlatform.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"

// Sets default values
ARGBColorPlatform::ARGBColorPlatform()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	PlatformMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlatformMesh"));

	SetRootComponent(PlatformMesh);

	PlatformMesh->SetCollisionProfileName(TEXT("BlockAll"));
	PlatformMesh->SetSimulatePhysics(false);

	ColorComponent = CreateDefaultSubobject<URGBColorComponent>(TEXT("ColorComponent"));
}

void ARGBColorPlatform::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const ERGBColor DisplayColor = HasActorBegunPlay() ? ColorComponent->GetCurrentColor() : ColorComponent->GetInitialColor();

	ApplyColorMaterial(DisplayColor);
}

URGBColorComponent* ARGBColorPlatform::GetColorComponent() const
{
	return ColorComponent;
}

// Called when the game starts or when spawned
void ARGBColorPlatform::BeginPlay()
{
	Super::BeginPlay();

	ColorComponent->OnColorChanged.AddDynamic(this, &ARGBColorPlatform::HandleColorChange);

	ApplyColorMaterial(ColorComponent->GetCurrentColor());
	
}

void ARGBColorPlatform::HandleColorChange(ERGBColor PreviousColor, ERGBColor NewColor)
{
	ApplyColorMaterial(NewColor);
}

void ARGBColorPlatform::ApplyColorMaterial(ERGBColor Color)
{
	UMaterialInterface* SelectedMaterial = nullptr;

	switch (Color) {
	case ERGBColor::Red:
		SelectedMaterial = RedMaterial.Get();
		break;

	case ERGBColor::Green:
		SelectedMaterial = GreenMaterial.Get();
		break;

	case ERGBColor::Blue:
		SelectedMaterial = BlueMaterial.Get();
		break;
	}

	if (SelectedMaterial) {
		PlatformMesh->SetMaterial(ColorMaterialIndex, SelectedMaterial);
	}
	else if (HasActorBegunPlay()) {
		UE_LOG(LogTemp, Warning, TEXT("%s has no material assigned for color %d."), *GetName(), static_cast<int32>(Color));
	}
}

