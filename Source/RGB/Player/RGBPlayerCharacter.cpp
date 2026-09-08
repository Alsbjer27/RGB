// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBPlayerCharacter.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "../Camera/RGBSideViewCameraComponent.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"

// Sets default values
ARGBPlayerCharacter::ARGBPlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	// Initial collisionbox size (Change later)
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	// Movement on X and Z axis only
	Movement->SetPlaneConstraintNormal(FVector::YAxisVector);
	Movement->SetPlaneConstraintOrigin(FVector::ZeroVector);
	Movement->SetPlaneConstraintEnabled(true);
	Movement->bSnapToPlaneAtStart = true;

	// Turn off visual facing 
	Movement->bOrientRotationToMovement = false;
	Movement->bUseControllerDesiredRotation = false;

	Movement->MaxWalkSpeed = 650.0f;
	Movement->MaxAcceleration = 2048.0f;
	Movement->BrakingDecelerationWalking = 2048.0f;

	Movement->JumpZVelocity = 650.0f;
	Movement->GravityScale = 1.8f;
	Movement->AirControl = 0.35f;

	SideViewCameraArm = CreateDefaultSubobject<URGBSideViewCameraComponent>(TEXT("SideViewCameraArm"));
	SideViewCameraArm->SetupAttachment(GetRootComponent());

	SideViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCamera->SetupAttachment(SideViewCameraArm, USpringArmComponent::SocketName);

	SideViewCamera->ProjectionMode = ECameraProjectionMode::Perspective;
	SideViewCamera->FieldOfView = 60.0f;
	SideViewCamera->bUsePawnControlRotation = false;
}

// Called when the game starts or when spawned
void ARGBPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ARGBPlayerCharacter::Move(const FInputActionValue& Value)
{
	const float MoveAmount = FMath::Clamp(Value.Get<float>(), -1.0f, 1.0f);
	AddMovementInput(FVector::ForwardVector, MoveAmount);
}

void ARGBPlayerCharacter::StartJump()
{
	Jump();
}

void ARGBPlayerCharacter::EndJump()
{
	StopJumping();
}

// Called every frame
void ARGBPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ARGBPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);

	if (!ensureMsgf(EnhancedInput, TEXT("RGBPlayerCharacter requires Enhanced Input"))) {
		return;
	}

	if (!ensureMsgf(MoveAction, TEXT("MoveAction is not assigned"))) {
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARGBPlayerCharacter::Move);
	}

	if (!ensureMsgf(JumpAction, TEXT("JumpAction is not assigned"))) {
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ARGBPlayerCharacter::StartJump);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ARGBPlayerCharacter::EndJump);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ARGBPlayerCharacter::EndJump);
	}

}

