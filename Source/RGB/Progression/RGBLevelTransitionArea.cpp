#include "RGBLevelTransitionArea.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "../Player/RGBPlayerCharacter.h"
#include "../Player/RGBPlayerController.h"

ARGBLevelTransitionArea::ARGBLevelTransitionArea()
{
	PrimaryActorTick.bCanEverTick = false;

	TransitionTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("TransitionTrigger"));
	SetRootComponent(TransitionTrigger);
	TransitionTrigger->SetBoxExtent(FVector(100.0f, 250.0f, 150.0f));
	TransitionTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TransitionTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	TransitionTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TransitionTrigger->SetGenerateOverlapEvents(true);
	TransitionTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ARGBLevelTransitionArea::HandleTransitionOverlap);
}

void ARGBLevelTransitionArea::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(TransitionTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void ARGBLevelTransitionArea::HandleTransitionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bTransitionStarted || DestinationLevel.IsNull())
	{
		return;
	}

	ARGBPlayerCharacter* PlayerCharacter = Cast<ARGBPlayerCharacter>(OtherActor);
	APlayerController* PlayerController = IsValid(PlayerCharacter) ? Cast<APlayerController>(PlayerCharacter->GetController()) : nullptr;

	if (!IsValid(PlayerController) || !PlayerController->IsLocalController() || !IsValid(PlayerController->PlayerCameraManager))
	{
		return;
	}

	bTransitionStarted = true;
	TransitioningController = PlayerController;
	TransitionTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PlayerController->SetIgnoreMoveInput(true);
	PlayerController->SetIgnoreLookInput(true);

	const float SafeFadeOutDuration = FMath::Max(FadeOutDuration, 0.0f);
	const float SafeBlackHoldDuration = FMath::Max(BlackHoldDuration, 0.0f);

	if (SafeFadeOutDuration > 0.0f)
	{
		PlayerController->PlayerCameraManager->StartCameraFade(0.0f, 1.0f, SafeFadeOutDuration, FLinearColor::Black, false, true);
	}
	else
	{
		PlayerController->PlayerCameraManager->SetManualCameraFade(1.0f, FLinearColor::Black, false);
	}

	const float TransitionDelay = SafeFadeOutDuration + SafeBlackHoldDuration;

	if (TransitionDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(TransitionTimerHandle, this, &ARGBLevelTransitionArea::ShowCardOrOpenDestination, TransitionDelay, false);
	}
	else
	{
		ShowCardOrOpenDestination();
	}
}

void ARGBLevelTransitionArea::ShowCardOrOpenDestination()
{
	ARGBPlayerController* RGBController = Cast<ARGBPlayerController>(TransitioningController.Get());
	if ((!CardTitle.IsEmpty() || !CardText.IsEmpty()) && IsValid(RGBController)) {
		TWeakObjectPtr<ARGBLevelTransitionArea> WeakThis(this);
		const bool bShown = RGBController->ShowStoryCard(CardTitle, CardText, CardPromptText, CardFadeInDuration, CardPromptDelay, CardFadeOutDuration,
			[WeakThis]() {
				if (WeakThis.IsValid()) {
					WeakThis->OpenDestinationLevel();
				}
			});
		if (bShown) {
			return;
		}
	}
	OpenDestinationLevel();
}

void ARGBLevelTransitionArea::OpenDestinationLevel()
{
	if (DestinationLevel.IsNull())
	{
		bTransitionStarted = false;
		TransitionTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		return;
	}

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, DestinationLevel);
}
