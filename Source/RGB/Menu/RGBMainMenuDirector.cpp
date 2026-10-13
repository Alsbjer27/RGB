#include "RGBMainMenuDirector.h"

#include "../Platforms/RGBColorPlatform.h"
#include "../Player/RGBPlayerCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

ARGBMainMenuDirector::ARGBMainMenuDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void ARGBMainMenuDirector::BeginPlay()
{
	Super::BeginPlay();

	if (!IsValid(MenuCharacter))
	{
		MenuCharacter = Cast<ARGBPlayerCharacter>(
			UGameplayStatics::GetPlayerCharacter(this, 0));
	}

	if (bStartCreditsSequenceOnBeginPlay)
	{
		StartCreditsSequence();
	}
}

bool ARGBMainMenuDirector::StartStartSequence()
{
	return StartSequence(
		StartMovementSteps,
		ERGBColor::Red,
		TEXT("Start"));
}

bool ARGBMainMenuDirector::StartSettingsSequence()
{
	return StartSequence(
		SettingsMovementSteps,
		ERGBColor::Green,
		TEXT("Settings"));
}

bool ARGBMainMenuDirector::StartCreditsSequence()
{
	return StartSequence(
		CreditsMovementSteps,
		ERGBColor::Blue,
		TEXT("Credits"));
}

bool ARGBMainMenuDirector::StartSequence(
	const TArray<FRGBMenuMovementStep>& MovementSteps,
	const ERGBColor RequiredColor,
	const TCHAR* SequenceName)
{
	if (bSequenceRunning)
	{
		return false;
	}

	if (!IsValid(MenuCharacter) ||
		MovementSteps.IsEmpty() ||
		DemonstrationPlatforms.IsEmpty())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s cannot start %s sequence: assign the "
				"character, platforms and movement steps."),
			*GetName(),
			SequenceName);

		return false;
	}

	ActiveMovementSteps = &MovementSteps;
	ActiveSequenceName = SequenceName;
	RequiredFinalColor = RequiredColor;

	bSequenceRunning = true;
	CurrentStepIndex = 0;

	SetActorTickEnabled(true);
	BeginCurrentStep();

	return true;
}

void ARGBMainMenuDirector::BeginCurrentStep()
{
	if (ActiveMovementSteps == nullptr ||
		!ActiveMovementSteps->IsValidIndex(CurrentStepIndex))
	{
		FinishSequence();
		return;
	}

	const FRGBMenuMovementStep& Step =
		(*ActiveMovementSteps)[CurrentStepIndex];

	if (!IsValid(Step.TargetMarker))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s has an invalid target in %s step %d."),
			*GetName(),
			ActiveSequenceName,
			CurrentStepIndex);

		if (IsValid(MenuCharacter))
		{
			MenuCharacter->StopScriptedJump();
		}

		bSequenceRunning = false;
		ActiveMovementSteps = nullptr;
		CurrentStepIndex = INDEX_NONE;

		SetActorTickEnabled(false);
		return;
	}

	JumpHoldRemaining = 0.0f;
	PauseRemaining =
		FMath::Max(Step.PauseAfterArrival, 0.0f);

	bObservedAirborne = false;
	bReachedCurrentStep = false;

	if (Step.bJumpAtStart)
	{
		JumpHoldRemaining =
			FMath::Max(Step.JumpHoldDuration, 0.0f);

		MenuCharacter->StartScriptedJump();
	}
}

void ARGBMainMenuDirector::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bSequenceRunning || !IsValid(MenuCharacter))
	{
		return;
	}

	if (bReturningToStart)
	{
		TickReturnToMenuStart();
		return;
	}

	if (ActiveMovementSteps == nullptr || !ActiveMovementSteps->IsValidIndex(CurrentStepIndex))
	{
		return;
	}

	const FRGBMenuMovementStep& Step =
		(*ActiveMovementSteps)[CurrentStepIndex];

	if (!IsValid(Step.TargetMarker))
	{
		return;
	}

	UCharacterMovementComponent* Movement =
		MenuCharacter->GetCharacterMovement();

	if (!IsValid(Movement))
	{
		return;
	}

	if (Step.bJumpAtStart &&
		JumpHoldRemaining > 0.0f)
	{
		JumpHoldRemaining -= DeltaTime;

		if (JumpHoldRemaining <= 0.0f)
		{
			MenuCharacter->StopScriptedJump();
		}
	}

	if (Movement->IsFalling())
	{
		bObservedAirborne = true;
	}

	if (!bReachedCurrentStep)
	{
		const float CharacterX =
			MenuCharacter->GetActorLocation().X;

		const float TargetX =
			Step.TargetMarker->GetActorLocation().X;

		const float DifferenceX =
			TargetX - CharacterX;

		const float SafeAcceptanceRadius =
			FMath::Max(Step.AcceptanceRadius, 1.0f);

		const bool bReachedX =
			FMath::Abs(DifferenceX) <=
			SafeAcceptanceRadius;

		if (!bReachedX)
		{
			const float MoveDirection =
				DifferenceX > 0.0f ? 1.0f : -1.0f;

			MenuCharacter->ApplyScriptedHorizontalInput(
				MoveDirection);

			return;
		}

		bool bLandingRequirementMet = true;

		if (Step.bWaitUntilLanded)
		{
			if (Step.bJumpAtStart)
			{
				bLandingRequirementMet =
					bObservedAirborne &&
					Movement->IsMovingOnGround();
			}
			else
			{
				bLandingRequirementMet =
					Movement->IsMovingOnGround();
			}
		}

		if (!bLandingRequirementMet)
		{
			return;
		}

		bReachedCurrentStep = true;

		if (PauseRemaining <= 0.0f)
		{
			AdvanceToNextStep();
		}

		return;
	}

	PauseRemaining =
		FMath::Max(
			0.0f,
			PauseRemaining - DeltaTime);

	if (PauseRemaining <= 0.0f)
	{
		AdvanceToNextStep();
	}
}

void ARGBMainMenuDirector::AdvanceToNextStep()
{
	++CurrentStepIndex;

	if (ActiveMovementSteps == nullptr ||
		!ActiveMovementSteps->IsValidIndex(CurrentStepIndex))
	{
		FinishSequence();
		return;
	}

	BeginCurrentStep();
}

void ARGBMainMenuDirector::FinishSequence()
{
	if (IsValid(MenuCharacter))
	{
		MenuCharacter->StopScriptedJump();
	}

	CurrentStepIndex = INDEX_NONE;
	ActiveMovementSteps = nullptr;

	if (!AreAllPlatformsColor(RequiredFinalColor))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s finished the %s movement, but the demonstration platforms do not have the required color."), *GetName(), ActiveSequenceName);

		bSequenceRunning = false;
		ActiveSequenceName = TEXT("None");
		SetActorTickEnabled(false);
		return;
	}

	if (!IsValid(MenuReturnMarker))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s has no MenuReturnMarker. Completing the sequence without walking back."), *GetName());

		CompleteSequence();
		return;
	}

	bReturningToStart = true;
}

bool ARGBMainMenuDirector::AreAllPlatformsColor(
	const ERGBColor RequiredColor) const
{
	if (DemonstrationPlatforms.IsEmpty())
	{
		return false;
	}

	for (ARGBColorPlatform* Platform :
		DemonstrationPlatforms)
	{
		if (!IsValid(Platform))
		{
			return false;
		}

		const URGBColorComponent* Color =
			Platform->GetColorComponent();

		if (!IsValid(Color) ||
			Color->GetCurrentColor() != RequiredColor)
		{
			return false;
		}
	}

	return true;
}

void ARGBMainMenuDirector::TickReturnToMenuStart()
{
	if (!IsValid(MenuCharacter) || !IsValid(MenuReturnMarker))
	{
		CompleteSequence();
		return;
	}

	const float CharacterX = MenuCharacter->GetActorLocation().X;
	const float TargetX = MenuReturnMarker->GetActorLocation().X;
	const float DifferenceX = TargetX - CharacterX;
	const float SafeAcceptanceRadius = FMath::Max(ReturnAcceptanceRadius, 1.0f);

	if (FMath::Abs(DifferenceX) > SafeAcceptanceRadius)
	{
		const float MoveDirection = DifferenceX > 0.0f ? 1.0f : -1.0f;
		MenuCharacter->ApplyScriptedHorizontalInput(MoveDirection);
		return;
	}

	if (UCharacterMovementComponent* Movement = MenuCharacter->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	CompleteSequence();
}

void ARGBMainMenuDirector::CompleteSequence()
{
	const ERGBColor FinishedColor = RequiredFinalColor;

	ResetDemonstartationPlatforms();

	bReturningToStart = false;
	bSequenceRunning = false;
	ActiveMovementSteps = nullptr;
	ActiveSequenceName = TEXT("None");
	CurrentStepIndex = INDEX_NONE;

	SetActorTickEnabled(false);

	switch (FinishedColor)
	{
	case ERGBColor::Red:
		OnStartReady.Broadcast();
		break;

	case ERGBColor::Green:
		OnSettingsReady.Broadcast();
		break;

	case ERGBColor::Blue:
		OnCreditsReady.Broadcast();
		break;

	default:
		break;
	}
}

void ARGBMainMenuDirector::ResetDemonstartationPlatforms()
{
	for (ARGBColorPlatform* Platform : DemonstrationPlatforms) {
		if (!IsValid(Platform)) {
			continue;
		}

		if (URGBColorComponent* ColorComponent = Platform->GetColorComponent()) {
			ColorComponent->ResetColor();
		}
	}
}
