#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "RGBUpgradeHUDCommandlet.generated.h"

/** One-time, editor-only migration of the prototype HUD's arena display. */
UCLASS()
class URGBUpgradeHUDCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    virtual int32 Main(const FString& Params) override;
};
