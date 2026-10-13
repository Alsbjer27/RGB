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

private:
	UFUNCTION()
	void HandleTransitionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void OpenDestinationLevel();

	FTimerHandle TransitionTimerHandle;
	bool bTransitionStarted = false;
};
