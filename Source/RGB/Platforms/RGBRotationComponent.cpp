#include "RGBRotationComponent.h"

#include "GameFramework/Actor.h"

URGBRotationComponent::URGBRotationComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void URGBRotationComponent::BeginPlay()
{
	Super::BeginPlay();

	if (RotationAxis.IsNearlyZero())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("%s has an invalid zero rotation axis."),
			*GetNameSafe(GetOwner()));

		SetRotationEnabled(false);
		return;
	}

	RemainingStepDegrees = FMath::Abs(StepAngle);
	RemainingDelay = 0.0f;

	SetRotationEnabled(bStartActive);
}

void URGBRotationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bRotationEnabled || RotationSpeed <= 0.0f)
	{
		return;
	}

	switch (RotationMode)
	{
	case ERGBRotationMode::Continuous:
		RotateByDegrees(RotationSpeed * DeltaTime);
		break;

	case ERGBRotationMode::Stepped:
		if (RemainingStepDegrees <= KINDA_SMALL_NUMBER)
		{
			if (RemainingDelay > 0.0f)
			{
				RemainingDelay -= DeltaTime;
				return;
			}

			RemainingStepDegrees = FMath::Abs(StepAngle);

			if (RemainingStepDegrees <= KINDA_SMALL_NUMBER)
			{
				return;
			}
		}

		{
			const float DegreesThisFrame = FMath::Min(RotationSpeed * DeltaTime, RemainingStepDegrees);
			const float RotationSign = StepAngle < 0.0f ? -1.0f : 1.0f;

			RotateByDegrees(DegreesThisFrame * RotationSign);

			RemainingStepDegrees -= DegreesThisFrame;

			if (RemainingStepDegrees <= KINDA_SMALL_NUMBER)
			{
				RemainingStepDegrees = 0.0f;
				RemainingDelay = DelayBetweenSteps;
			}
		}
		break;
	}
}

void URGBRotationComponent::SetRotationEnabled(bool bEnabled)
{
	bRotationEnabled = bEnabled && !RotationAxis.IsNearlyZero();
	SetComponentTickEnabled(bRotationEnabled);
}

bool URGBRotationComponent::IsRotationEnabled() const
{
	return bRotationEnabled;
}

void URGBRotationComponent::RotateByDegrees(float Degrees)
{
	AActor* Owner = GetOwner();

	if (!IsValid(Owner))
	{
		return;
	}

	const FVector SafeAxis = RotationAxis.GetSafeNormal();
	const FQuat DeltaRotation(SafeAxis, FMath::DegreesToRadians(Degrees));

	Owner->AddActorLocalRotation(DeltaRotation, false, nullptr, ETeleportType::None);
}