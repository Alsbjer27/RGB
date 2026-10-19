#include "RGBHUDWidget.h"

#include "../Arena/RGBArenaControl.h"
#include "../Camera/RGBArenaCameraZone.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"

void URGBHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    // The character Blueprint creates a fresh HUD on respawn. Retire its previous viewport instance.
    TArray<UUserWidget*> ExistingHUDs;
    UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, ExistingHUDs, StaticClass(), true);
    for (UUserWidget* ExistingHUD : ExistingHUDs)
    {
        if (ExistingHUD != this && ExistingHUD->GetOwningPlayer() == GetOwningPlayer())
        {
            ExistingHUD->RemoveFromParent();
        }
    }
    Arenas.Reset();
    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<ARGBArenaControl> It(World); It; ++It)
        {
            RegisterArena(*It);
        }
        ActorSpawnedHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &URGBHUDWidget::RegisterArena));
    }
    if (UImage* LiquidImage = Cast<UImage>(GetWidgetFromName(TEXT("Image_ArenaLiquid"))))
    {
        ArenaLiquidMaterial = LiquidImage->GetDynamicMaterial();
    }
    ensureMsgf(ArenaLiquidMaterial, TEXT("%s requires Image_ArenaLiquid with a material exposing FillAmount."), *GetName());
    DisplayedArena.Reset();
    ArenaProgressDisplayed = 0.0f;
    UpdateArenaPresentation(0.0f);
}

void URGBHUDWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld(); World && ActorSpawnedHandle.IsValid())
    {
        World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
    }
    ActorSpawnedHandle.Reset();
    Arenas.Reset();
    DisplayedArena.Reset();
    ArenaLiquidMaterial = nullptr;
    Super::NativeDestruct();
}

void URGBHUDWidget::RegisterArena(AActor* Actor)
{
    if (ARGBArenaControl* Arena = Cast<ARGBArenaControl>(Actor))
    {
        Arenas.AddUnique(Arena);
    }
}

void URGBHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    UpdateArenaPresentation(InDeltaTime);
}

void URGBHUDWidget::UpdateArenaPresentation(float DeltaTime)
{
    ARGBArenaControl* ActiveArena = nullptr;
    if (const APawn* Pawn = GetOwningPlayerPawn())
    {
        Arenas.RemoveAll([](const TWeakObjectPtr<ARGBArenaControl>& Arena) { return !Arena.IsValid(); });
        for (const TWeakObjectPtr<ARGBArenaControl>& Arena : Arenas)
        {
            const ARGBArenaCameraZone* Zone = Arena->GetArenaCameraZone();
            if (IsValid(Zone) && Zone->ContainWorldLocation(Pawn->GetActorLocation()))
            {
                ActiveArena = Arena.Get();
                break;
            }
        }
    }
    ArenaProgressTarget = ActiveArena ? ActiveArena->GetCompletionFraction() : 0.0f;
    // Never carry the previous room's fill into another room or a connector.
    if (DisplayedArena.Get() != ActiveArena || !ActiveArena)
    {
        ArenaProgressDisplayed = ArenaProgressTarget;
    }
    else
    {
        ArenaProgressDisplayed = FMath::FInterpTo(ArenaProgressDisplayed, ArenaProgressTarget, DeltaTime, ArenaProgressSmoothingSpeed);
    }
    DisplayedArena = ActiveArena;
    if (ArenaLiquidMaterial)
    {
        ArenaLiquidMaterial->SetScalarParameterValue(TEXT("FillAmount"), ArenaProgressDisplayed);
    }
}
