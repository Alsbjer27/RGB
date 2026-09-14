#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RGBColorComponent.generated.h"

UENUM(BlueprintType)
enum class ERGBColor : uint8
{
    Red,
    Green,
    Blue
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FRGBColorChangedSignature,
    ERGBColor, PreviousColor,
    ERGBColor, NewColor);

UCLASS(ClassGroup = RGB, meta = (BlueprintSpawnableComponent))
class RGB_API URGBColorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    URGBColorComponent();

    ERGBColor GetCurrentColor() const;
    ERGBColor GetInitialColor() const;

    bool SetColor(ERGBColor NewColor);
    void AdvanceColor();
    void ResetColor();
    void SetColorLocked(bool bLocked);

    UPROPERTY(BlueprintAssignable, Category = "RGB|Color")
    FRGBColorChangedSignature OnColorChanged;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Color")
    bool bRandomizeInitialColor = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Color")
    ERGBColor InitialColor = ERGBColor::Red;

private:
    UPROPERTY(VisibleInstanceOnly, Category = "RGB|Color")
    ERGBColor CurrentColor = ERGBColor::Red;

    UPROPERTY(Transient, VisibleInstanceOnly, Category = "RGB|Color")
    ERGBColor StartingColor = ERGBColor::Red;

    UPROPERTY(Transient, VisibleInstanceOnly, Category = "RGB|Color")
    bool bColorLocked = false;
};