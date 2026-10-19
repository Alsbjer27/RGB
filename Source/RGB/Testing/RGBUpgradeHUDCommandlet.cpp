#include "RGBUpgradeHUDCommandlet.h"

#if WITH_EDITOR
#include "../Player/RGBHUDWidget.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphSchema_K2.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#endif

int32 URGBUpgradeHUDCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/RGB/UI/HUD/WBP_RGBHUD_Prototype.WBP_RGBHUD_Prototype"));
    if (!Blueprint) { return 1; }
    if (Blueprint->ParentClass == URGBHUDWidget::StaticClass())
    {
        UE_LOG(LogTemp, Display, TEXT("RGB_HUD_UPGRADE: already migrated; no changes."));
        return 0;
    }
    UEdGraph* Graph = FBlueprintEditorUtils::FindEventGraph(Blueprint);
    if (!Graph) { return 2; }
    // Preserve the existing health presentation and its exact wiring. Replace only arena presentation.
    const TSet<FName> Keep = {
        TEXT("K2Node_Event_3"), TEXT("K2Node_CallFunction_4"), TEXT("K2Node_DynamicCast_1"),
        TEXT("K2Node_VariableSet_6"), TEXT("K2Node_AddDelegate_1"), TEXT("K2Node_CustomEvent_1"),
        TEXT("K2Node_CallFunction_8"), TEXT("K2Node_CallFunction_7"), TEXT("K2Node_VariableGet_2"), TEXT("K2Node_CallFunction_9")
    };
    UEdGraphNode* Construct = nullptr;
    UEdGraphNode* HealthCast = nullptr;
    TSet<FName> Found;
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Keep.Contains(Node->GetFName())) { Found.Add(Node->GetFName()); }
        if (Node->GetFName() == TEXT("K2Node_Event_3")) { Construct = Node; }
        if (Node->GetFName() == TEXT("K2Node_DynamicCast_1")) { HealthCast = Node; }
    }
    if (Found.Num() != Keep.Num() || !Construct || !HealthCast)
    {
        UE_LOG(LogTemp, Error, TEXT("RGB_HUD_UPGRADE: HUD layout differs from the audited prototype; nothing saved."));
        return 3;
    }
    UEdGraphPin* Start = Construct->FindPin(UEdGraphSchema_K2::PN_Then);
    UEdGraphPin* Health = HealthCast->FindPin(UEdGraphSchema_K2::PN_Execute);
    if (!Start || !Health) { return 4; }
    const TArray<TObjectPtr<UEdGraphNode>> OldNodes = Graph->Nodes;
    for (UEdGraphNode* Node : OldNodes)
    {
        if (!Keep.Contains(Node->GetFName()))
        {
            FBlueprintEditorUtils::RemoveNode(Blueprint, Node, true);
        }
    }
    Start->BreakAllPinLinks();
    Health->BreakAllPinLinks();
    if (!Graph->GetSchema()->TryCreateConnection(Start, Health)) { return 5; }
    if (UEdGraph* ProgressGraph = FindObject<UEdGraph>(Blueprint, TEXT("UpdateArenaProgress")))
    {
        FBlueprintEditorUtils::RemoveGraph(Blueprint, ProgressGraph);
    }
    for (const FName Variable : { FName(TEXT("ArenaControl")), FName(TEXT("ArenaLiquidMaterial")), FName(TEXT("TargetArenaProgress")), FName(TEXT("DisplayedArenaProgress")), FName(TEXT("ProgressInterpSpeed")) })
    {
        FBlueprintEditorUtils::RemoveMemberVariable(Blueprint, Variable);
    }
    Blueprint->ParentClass = URGBHUDWidget::StaticClass();
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    if (Blueprint->Status == BS_Error) { return 6; }
    UPackage* Package = Blueprint->GetOutermost();
    const FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    const bool bSaved = UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs);
    UE_LOG(LogTemp, Display, TEXT("RGB_HUD_UPGRADE: %s"), bSaved ? TEXT("saved; health graph preserved; arena presentation now native") : TEXT("save failed"));
    return bSaved ? 0 : 7;
#else
    return 1;
#endif
}

