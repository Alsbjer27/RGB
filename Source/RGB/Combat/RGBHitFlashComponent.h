// Brief white overlay flash used as hit feedback for the player and enemies.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RGBHitFlashComponent.generated.h"

class UMaterialInterface;
class UMeshComponent;

/**
 * Presentation-only hit feedback. Flash() places FlashMaterial as the overlay material on
 * every visible mesh of the owning actor for FlashDuration seconds, then restores the
 * previous overlay. Base/colour materials are never replaced, so colour logic is unaffected.
 */
UCLASS(ClassGroup = (RGB), meta = (BlueprintSpawnableComponent))
class RGB_API URGBHitFlashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URGBHitFlashComponent();

	/** Starts the flash, or restarts its timer if a flash is already showing. */
	UFUNCTION(BlueprintCallable, Category = "RGB|Hit Flash")
	void Flash();

	/** Removes the flash immediately and restores the previous overlay materials. */
	UFUNCTION(BlueprintCallable, Category = "RGB|Hit Flash")
	void StopFlash();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Overlay material shown during the flash, e.g. an unlit translucent white material. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Hit Flash")
	TObjectPtr<UMaterialInterface> FlashMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Hit Flash", meta = (ClampMin = "0.01", Units = "s"))
	float FlashDuration = 0.08f;

private:
	FTimerHandle FlashTimer;
	TArray<TWeakObjectPtr<UMeshComponent>> FlashedMeshes;
	TArray<TObjectPtr<UMaterialInterface>> PreviousOverlayMaterials;
	bool bWarnedMissingMaterial = false;
};
