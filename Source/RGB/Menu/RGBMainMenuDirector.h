#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "../Platforms/RGBColorComponent.h"
#include "RGBMainMenuDirector.generated.h"

class ARGBColorPlatform;
class ARGBPlayerCharacter;

USTRUCT(BlueprintType)
struct FRGBMenuMovementStep
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	TObjectPtr<AActor> TargetMarker;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bJumpAtStart = false;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		meta = (
			ClampMin = "0.0",
			EditCondition = "bJumpAtStart",
			Units = "s"))
	float JumpHoldDuration = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bWaitUntilLanded = true;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		meta = (ClampMin = "1.0", Units = "cm"))
	float AcceptanceRadius = 25.0f;

	UPROPERTY(
		EditAnywhere,
		BlueprintReadOnly,
		meta = (ClampMin = "0.0", Units = "s"))
	float PauseAfterArrival = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(
	FRGBMenuStartReadySignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(
	FRGBMenuSettingsReadySignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(
	FRGBMenuCreditsReadySignature);

UCLASS()
class RGB_API ARGBMainMenuDirector : public AActor
{
	GENERATED_BODY()

public:
	ARGBMainMenuDirector();

	virtual void Tick(float DeltaTime) override;

	bool StartStartSequence();
	bool StartSettingsSequence();
	bool StartCreditsSequence();

	bool IsSequenceRunning() const
	{
		return bSequenceRunning;
	}

	UPROPERTY(BlueprintAssignable, Category = "RGB|Main Menu")
	FRGBMenuStartReadySignature OnStartReady;

	UPROPERTY(BlueprintAssignable, Category = "RGB|Main Menu")
	FRGBMenuSettingsReadySignature OnSettingsReady;

	UPROPERTY(BlueprintAssignable, Category = "RGB|Main Menu")
	FRGBMenuCreditsReadySignature OnCreditsReady;

protected:
	virtual void BeginPlay() override;

	UPROPERTY( EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Main Menu")
	TObjectPtr<ARGBPlayerCharacter> MenuCharacter;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Main Menu")
	TObjectPtr<AActor> MenuReturnMarker;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Main Menu", meta = (ClampMin = "1.0", Units = "cm"))
	float ReturnAcceptanceRadius = 25.0f;

	UPROPERTY( EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Main Menu")
	TArray<TObjectPtr<ARGBColorPlatform>> DemonstrationPlatforms;

	UPROPERTY( EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Main Menu|Start")
	TArray<FRGBMenuMovementStep> StartMovementSteps;

	UPROPERTY( EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Main Menu|Settings")
	TArray<FRGBMenuMovementStep> SettingsMovementSteps;

	UPROPERTY( EditInstanceOnly, BlueprintReadOnly, Category = "RGB|Main Menu|Credits")
	TArray<FRGBMenuMovementStep> CreditsMovementSteps;

	UPROPERTY( EditAnywhere, BlueprintReadOnly, Category = "RGB|Main Menu|Testing")
	bool bStartCreditsSequenceOnBeginPlay = false;

private:
	bool StartSequence( const TArray<FRGBMenuMovementStep>& MovementSteps, ERGBColor RequiredColor, const TCHAR* SequenceName);

	void BeginCurrentStep();
	void AdvanceToNextStep();
	void FinishSequence();

	void TickReturnToMenuStart();
	void CompleteSequence();

	void ResetDemonstartationPlatforms();

	bool AreAllPlatformsColor(ERGBColor RequiredColor) const;

	const TArray<FRGBMenuMovementStep>* ActiveMovementSteps = nullptr;

	const TCHAR* ActiveSequenceName = TEXT("None");

	ERGBColor RequiredFinalColor = ERGBColor::Red;

	int32 CurrentStepIndex = INDEX_NONE;

	float JumpHoldRemaining = 0.0f;
	float PauseRemaining = 0.0f;

	bool bSequenceRunning = false;
	bool bObservedAirborne = false;
	bool bReachedCurrentStep = false;
	bool bReturningToStart = false;
};