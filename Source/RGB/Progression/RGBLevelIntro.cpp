// Per-level story card shown on black before the level fades in.

#include "RGBLevelIntro.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"

ARGBLevelIntro::ARGBLevelIntro()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot")));
#if WITH_EDITORONLY_DATA
	EditorIcon = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("EditorIcon"));
	if (EditorIcon) {
		EditorIcon->SetupAttachment(GetRootComponent());
	}
#endif
}
