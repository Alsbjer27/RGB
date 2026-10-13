// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include <Engine/EngineTypes.h>
#include "RGBDashAfterimageComponent.generated.h"

class UMaterialInterface;

UCLASS( ClassGroup=(RGB), meta=(BlueprintSpawnableComponent) )
class RGB_API URGBDashAfterimageComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	URGBDashAfterimageComponent();

	void StartTrail();
	void StopTrail();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Dash Afterimage")
	TObjectPtr<UMaterialInterface> AfterimageMaterial;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Dash Afterimage", meta = (ClampMin = "0.01", Units = "s"))
	float SpawnInterval = 0.04f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Dash Afterimage", meta = (ClampMin = "0.01", Units = "s"))
	float TrailDuration = 0.3f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Dash Afterimage", meta = (ClampMin = "0.01", Units = "s"))
	float AfterimageLifetime = 0.2f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Dash Afterimage", meta = (ClampMin = "0.01", Units = "s"))
	float InitialOpacity = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Dash Afterimage")
	FLinearColor AfterimageColor = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);

private:
	void SpawnAfterimage();

	FTimerHandle SpawnTimerHandle;
	FTimerHandle StopTimerHandle;

	bool bTrailActive = false;
	
};
