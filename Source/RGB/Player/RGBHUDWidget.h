#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RGBHUDWidget.generated.h"

class ARGBArenaControl;
class UMaterialInstanceDynamic;

/** Arena progress presentation. The arena control remains the source of completion state. */
UCLASS(Abstract)
class RGB_API URGBHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "RGB|HUD")
    ARGBArenaControl* GetDisplayedArena() const { return DisplayedArena.Get(); }

    UFUNCTION(BlueprintPure, Category = "RGB|HUD")
    float GetArenaProgressTarget() const { return ArenaProgressTarget; }

    UFUNCTION(BlueprintPure, Category = "RGB|HUD")
    float GetArenaProgressDisplayed() const { return ArenaProgressDisplayed; }

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(EditDefaultsOnly, Category = "RGB|HUD", meta = (ClampMin = "0.0"))
    float ArenaProgressSmoothingSpeed = 5.0f;

private:
    void RegisterArena(AActor* Actor);
    void UpdateArenaPresentation(float DeltaTime);

    TArray<TWeakObjectPtr<ARGBArenaControl>> Arenas;
    TWeakObjectPtr<ARGBArenaControl> DisplayedArena;
    FDelegateHandle ActorSpawnedHandle;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ArenaLiquidMaterial;

    float ArenaProgressTarget = 0.0f;
    float ArenaProgressDisplayed = 0.0f;
};
