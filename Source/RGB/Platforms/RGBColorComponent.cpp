#include "RGBColorComponent.h"

URGBColorComponent::URGBColorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void URGBColorComponent::BeginPlay()
{
    Super::BeginPlay();

    ResetColor();
}

ERGBColor URGBColorComponent::GetCurrentColor() const
{
    return CurrentColor;
}

ERGBColor URGBColorComponent::GetInitialColor() const
{
    return InitialColor;
}

bool URGBColorComponent::SetColor(ERGBColor NewColor)
{
    switch (NewColor)
    {
    case ERGBColor::Red:
    case ERGBColor::Green:
    case ERGBColor::Blue:
        break;

    default:
        ensureMsgf(false, TEXT("RGBColorComponent received an invalid color."));
        return false;
    }

    if (CurrentColor == NewColor)
    {
        return false;
    }

    const ERGBColor PreviousColor = CurrentColor;
    CurrentColor = NewColor;

    OnColorChanged.Broadcast(PreviousColor, CurrentColor);

    return true;
}

void URGBColorComponent::AdvanceColor()
{
    switch (CurrentColor)
    {
    case ERGBColor::Red:
        SetColor(ERGBColor::Green);
        break;

    case ERGBColor::Green:
        SetColor(ERGBColor::Blue);
        break;

    case ERGBColor::Blue:
        SetColor(ERGBColor::Red);
        break;
    }
}

void URGBColorComponent::ResetColor()
{
    StartingColor = InitialColor;

    if (bRandomizeInitialColor) {
		const ERGBColor AvailableVColors[] = { ERGBColor::Red, ERGBColor::Green, ERGBColor::Blue };
		const int32 RandomIndex = FMath::RandRange(0, 2);
		StartingColor = AvailableVColors[RandomIndex];
    }
    SetColor(StartingColor);
}