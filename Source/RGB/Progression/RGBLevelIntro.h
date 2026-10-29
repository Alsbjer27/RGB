// Per-level story card shown on black before the level fades in.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RGBLevelIntro.generated.h"

class UBillboardComponent;

/**
 * Place one in a level to show a title and narrative text when the level is loaded (not on
 * respawn). The player controller pauses the game, shows the text, and continues into the normal
 * level fade-in when the player presses the instruction "continue" input.
 */
UCLASS(meta = (DisplayName = "RGB Level Intro"))
class RGB_API ARGBLevelIntro : public AActor
{
	GENERATED_BODY()

public:
	ARGBLevelIntro();

	/** Big heading, e.g. "ASSEMBLY HALL 03 - THE CHROMA WORKS". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Level Intro")
	FText Title;

	/** Narrative text under the heading. Use Shift+Enter for new lines. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Level Intro", meta = (MultiLine = "true"))
	FText Body;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Level Intro", meta = (ClampMin = "0.0", Units = "s"))
	float FadeInDuration = 1.5f;

	/** Seconds from the start of the fade-in until the continue prompt appears. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Level Intro", meta = (ClampMin = "0.0", Units = "s"))
	float PromptDelay = 3.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Level Intro", meta = (ClampMin = "0.0", Units = "s"))
	float FadeOutDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RGB|Level Intro")
	FText PromptText = NSLOCTEXT("RGB", "LevelIntroPrompt", "PRESS  A / SPACE  TO CONTINUE");

#if WITH_EDITORONLY_DATA
private:
	UPROPERTY()
	TObjectPtr<UBillboardComponent> EditorIcon;
#endif
};
