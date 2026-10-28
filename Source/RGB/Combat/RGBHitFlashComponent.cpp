// Brief white overlay flash used as hit feedback for the player and enemies.

#include "RGBHitFlashComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"

URGBHitFlashComponent::URGBHitFlashComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void URGBHitFlashComponent::Flash()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!IsValid(Owner) || !World) {
		return;
	}

	if (!IsValid(FlashMaterial)) {
		if (!bWarnedMissingMaterial) {
			UE_LOG(LogTemp, Warning, TEXT("%s: Hit Flash has no Flash Material assigned."), *GetNameSafe(Owner));
			bWarnedMissingMaterial = true;
		}
		return;
	}

	// Only store the original overlays once, so a hit during a flash just extends it.
	if (FlashedMeshes.IsEmpty()) {
		TArray<UMeshComponent*> Meshes;
		Owner->GetComponents<UMeshComponent>(Meshes);
		for (UMeshComponent* Mesh : Meshes) {
			if (!IsValid(Mesh) || !Mesh->IsVisible()) {
				continue;
			}
			FlashedMeshes.Add(Mesh);
			PreviousOverlayMaterials.Add(Mesh->GetOverlayMaterial());
			Mesh->SetOverlayMaterial(FlashMaterial);
		}
	}

	if (FlashedMeshes.IsEmpty()) {
		return;
	}
	World->GetTimerManager().SetTimer(FlashTimer, this, &URGBHitFlashComponent::StopFlash, FMath::Max(FlashDuration, 0.01f), false);
}

void URGBHitFlashComponent::StopFlash()
{
	if (UWorld* World = GetWorld()) {
		World->GetTimerManager().ClearTimer(FlashTimer);
	}
	for (int32 Index = 0; Index < FlashedMeshes.Num(); ++Index) {
		if (UMeshComponent* Mesh = FlashedMeshes[Index].Get()) {
			Mesh->SetOverlayMaterial(PreviousOverlayMaterials.IsValidIndex(Index) ? PreviousOverlayMaterials[Index] : nullptr);
		}
	}
	FlashedMeshes.Reset();
	PreviousOverlayMaterials.Reset();
}

void URGBHitFlashComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopFlash();
	Super::EndPlay(EndPlayReason);
}
