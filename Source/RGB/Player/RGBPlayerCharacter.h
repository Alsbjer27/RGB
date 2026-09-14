// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../Platforms/RGBColorComponent.h"
#include "RGBPlayerCharacter.generated.h"

class UCameraComponent;
class URGBSideViewCameraComponent;

class UInputAction;
struct FInputActionValue;

class UDamageType;

class ARGBColorPlatform;

class ARGBProjectile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRGBWeaponColorChangedSignature, ERGBColor, NewColor);

UCLASS()
class RGB_API ARGBPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARGBPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

	UFUNCTION(BlueprintPure, Category = "RGB|Combat")
	ERGBColor GetSelectedWeaponColor() const { return SelectedWeaponColor; }

	UPROPERTY(BlueprintAssignable, Category = "RGB|Color")
	FRGBWeaponColorChangedSignature OnWeaponColorChanged;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void Landed(const FHitResult& Hit) override;

	virtual void OnJumped_Implementation() override;
	TWeakObjectPtr<ARGBColorPlatform> JumpSourcePlatform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Camera")
	TObjectPtr<URGBSideViewCameraComponent> SideViewCameraArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RGB|Camera")
	TObjectPtr<UCameraComponent> SideViewCamera;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsonly, BlueprintReadOnly, Category = "RGB|Input")
	TObjectPtr<UInputAction> DashAction;

	void Move(const FInputActionValue& Value);
	void StartJump();
	void EndJump();
	
	void StartDash();
	float LastFacingDirection = 1.0f;

	/* START: Jump Buffering */
	virtual void CheckJumpInput(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Movement", meta = (ClampMin = "0.0", Units = "s"))
	float JumpBufferDuration = 0.15f;

	double BufferedJumpExpiresAt = -1.0f;
	bool bJumpInputHeld = false;
	/* END: Jump Buffering*/

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Input")
	TObjectPtr<UInputAction> FireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Combat")
	TSubclassOf<ARGBProjectile> ProjectileClass;

	void Fire();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Input")
	TObjectPtr<UInputAction> NextWeaponColorAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RGB|Input")
	TObjectPtr<UInputAction> PreviousWeaponColorAction;


private:
	void NextWeaponColor();
	void PreviousWeaponColor();
	void SetSelectedWeaponColor(ERGBColor NewColor);

	UPROPERTY(Transient, VisibleInstanceOnly, Category = "RGB|Combat")
	ERGBColor SelectedWeaponColor = ERGBColor::Red;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
