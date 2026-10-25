#include "RGBHUDWidget.h"

#include "../Arena/RGBArenaControl.h"
#include "../Camera/RGBArenaCameraZone.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Engine/Texture2D.h"
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

    EnsureAbilityInstructionImage();
    if (Image_AbilityInstruction)
    {
        Image_AbilityInstruction->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (ARGBPlayerController* Controller = Cast<ARGBPlayerController>(GetOwningPlayer()))
    {
        InstructionController = Controller;
        Controller->OnAbilityInstructionChanged.AddUniqueDynamic(this, &URGBHUDWidget::HandleAbilityInstructionChanged);
        // A replacement HUD picks up an instruction that is already open instead of leaving the game paused with nothing shown.
        ERGBAbilityInstruction ActiveInstruction = ERGBAbilityInstruction::Dash;
        ERGBPlayerAbility ActiveAbility = ERGBPlayerAbility::Dash;
        if (Controller->GetActiveAbilityInstruction(ActiveInstruction, ActiveAbility))
        {
            HandleAbilityInstructionChanged(true, ActiveInstruction, ActiveAbility);
        }
    }
}

void URGBHUDWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld(); World && ActorSpawnedHandle.IsValid())
    {
        World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
    }
    ActorSpawnedHandle.Reset();
    if (ARGBPlayerController* Controller = InstructionController.Get())
    {
        Controller->OnAbilityInstructionChanged.RemoveDynamic(this, &URGBHUDWidget::HandleAbilityInstructionChanged);
    }
    InstructionController.Reset();
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

void URGBHUDWidget::HandleAbilityInstructionChanged(bool bVisible, ERGBAbilityInstruction Instruction, ERGBPlayerAbility Ability)
{
    if (!bVisible)
    {
        if (Image_AbilityInstruction)
        {
            Image_AbilityInstruction->SetVisibility(ESlateVisibility::Collapsed);
        }
        OnAbilityInstructionHidden();
        return;
    }

    UTexture2D* InstructionImage = DashInstructionImage;
    switch (Instruction)
    {
    case ERGBAbilityInstruction::FirstWeapon:
        InstructionImage = FirstWeaponInstructionImage;
        break;
    case ERGBAbilityInstruction::WeaponSwitching:
        InstructionImage = WeaponSwitchingInstructionImage;
        break;
    case ERGBAbilityInstruction::AdditionalWeapon:
        InstructionImage = AdditionalWeaponInstructionImage;
        break;
    default:
        break;
    }

    if (!Image_AbilityInstruction || !IsValid(InstructionImage))
    {
        UE_LOG(LogTemp, Warning, TEXT("%s cannot display ability instruction %d: missing image widget or texture. Press the dismiss input to continue."), *GetName(), static_cast<int32>(Instruction));
        return;
    }

    Image_AbilityInstruction->SetBrushFromTexture(InstructionImage, true);
    Image_AbilityInstruction->SetDesiredSizeOverride(FVector2D(InstructionImage->GetSizeX(), InstructionImage->GetSizeY()) * AbilityInstructionImageScale);
    Image_AbilityInstruction->SetVisibility(ESlateVisibility::HitTestInvisible);
    OnAbilityInstructionShown();
}

void URGBHUDWidget::EnsureAbilityInstructionImage()
{
    if (Image_AbilityInstruction || !WidgetTree)
    {
        return;
    }

    // Use the outermost canvas or overlay (the root when it is one) so the image centres on the screen.
    UPanelWidget* HostPanel = nullptr;
    WidgetTree->ForEachWidget([&HostPanel](UWidget* Widget)
    {
        if (!HostPanel && (Widget->IsA<UCanvasPanel>() || Widget->IsA<UOverlay>()))
        {
            HostPanel = Cast<UPanelWidget>(Widget);
        }
    });
    if (!HostPanel)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s (root %s) needs a Canvas Panel or Overlay, or an Image_AbilityInstruction widget, to show ability instructions."), *GetName(), *GetNameSafe(GetRootWidget()));
        return;
    }
    if (HostPanel != GetRootWidget())
    {
        UE_LOG(LogTemp, Log, TEXT("%s places ability instructions in %s instead of the root %s."), *GetName(), *HostPanel->GetName(), *GetNameSafe(GetRootWidget()));
    }

    Image_AbilityInstruction = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Image_AbilityInstruction"));
    if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(HostPanel))
    {
        if (UCanvasPanelSlot* ImageSlot = Canvas->AddChildToCanvas(Image_AbilityInstruction))
        {
            ImageSlot->SetAnchors(FAnchors(0.5f, 0.5f));
            ImageSlot->SetAlignment(FVector2D(0.5f, 0.5f));
            ImageSlot->SetPosition(FVector2D::ZeroVector);
            ImageSlot->SetAutoSize(true);
            // Draw above the arena and health presentation.
            ImageSlot->SetZOrder(100);
        }
    }
    else if (UOverlay* Overlay = Cast<UOverlay>(HostPanel))
    {
        // Overlay children draw in order, so the last child is on top.
        if (UOverlaySlot* ImageSlot = Overlay->AddChildToOverlay(Image_AbilityInstruction))
        {
            ImageSlot->SetHorizontalAlignment(HAlign_Center);
            ImageSlot->SetVerticalAlignment(VAlign_Center);
        }
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
