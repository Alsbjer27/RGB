#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGBLevelTransitionArea.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class UWorld;

UCLASS()
class RGB_API ARGBLevelTransitionArea : public AActor
{
	GENERATED_BODY()

public:
	ARGBLevelTransitionArea();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Level Transition")
	TObjectPtr<UBoxComponent> TransitionTrigger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Level Transition")
	TSoftObjectPtr<UWorld> DestinationLevel;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Level Transition", meta = (ClampMin = "0.0", Units = "s"))
	float FadeOutDuration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Level Transition", meta = (ClampMin = "0.0", Units = "s"))
	float BlackHoldDuration = 0.25f;

	/** Optional story card shown on black after the fade-out, before the destination loads (e.g. the game's ending). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Level Transition|Story Card")
	FText CardTitle;

	/** Leave Title and Text empty to go straight to the destination. Use Shift+Enter for new lines. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Level Transition|Story Card", meta = (MultiLine = "true"))
	FText CardText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Level Transition|Story Card")
	FText CardPromptText = NSLOCTEXT("RGB", "TransitionCardPrompt", "PRESS  A / SPACE  TO CONTINUE");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Level Transition|Story Card", meta = (ClampMin = "0.0", Units = "s"))
	float CardFadeInDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Level Transition|Story Card", meta = (ClampMin = "0.0", Units = "s"))
	float CardPromptDelay = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Level Transition|Story Card", meta = (ClampMin = "0.0", Units = "s"))
	float CardFadeOutDuration = 1.0f;

private:
	UFUNCTION()
	void HandleTransitionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void OpenDestinationLevel();
	void ShowCardOrOpenDestination();

	TWeakObjectPtr<APlayerController> TransitioningController;

	FTimerHandle TransitionTimerHandle;
	bool bTransitionStarted = false;
};
