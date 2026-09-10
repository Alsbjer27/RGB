// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBPlayerCharacter.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "../Camera/RGBSideViewCameraComponent.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "Engine/World.h"
#include "RGBCharacterMovementComponent.h"

#include "../Game/RGBGameMode.h"



// Sets default values
ARGBPlayerCharacter::ARGBPlayerCharacter(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer.SetDefaultSubobjectClass<URGBCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
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

	Movement->MaxWalkSpeed = 1000.0f;
	Movement->MaxAcceleration = 10000.0f;
	Movement->BrakingDecelerationWalking = 10000.0f;

	Movement->JumpZVelocity = 1500.0f;
	Movement->GravityScale = 1.8f;
	Movement->AirControl = 0.55f;

	JumpMaxHoldTime = 0.2f;
	JumpMaxCount = 1;

	Movement->bApplyGravityWhileJumping = true;

	SideViewCameraArm = CreateDefaultSubobject<URGBSideViewCameraComponent>(TEXT("SideViewCameraArm"));
	SideViewCameraArm->SetupAttachment(GetRootComponent());

	SideViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCamera->SetupAttachment(SideViewCameraArm, USpringArmComponent::SocketName);

	SideViewCamera->ProjectionMode = ECameraProjectionMode::Perspective;
	SideViewCamera->FieldOfView = 60.0f;
	SideViewCamera->bUsePawnControlRotation = false;
}

void ARGBPlayerCharacter::FellOutOfWorld(const UDamageType& DamageType)
{
	ARGBGameMode* GameMode = GetWorld()->GetAuthGameMode<ARGBGameMode>();

	if (GameMode && Controller) {
		if (GameMode->RespawnPlayer(Controller)) {
			return;
		}
	}

	Super::FellOutOfWorld(DamageType);
}

// Called when the game starts or when spawned
void ARGBPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void ARGBPlayerCharacter::Move(const FInputActionValue& Value)
{
	const float MoveAmount = FMath::Clamp(Value.Get<float>(), -1.0f, 1.0f);

	if (!FMath::IsNearlyZero(MoveAmount)) {
		LastFacingDirection = MoveAmount > 0.0f ? 1.0 : -1.0f;
	}

	AddMovementInput(FVector::ForwardVector, MoveAmount);
}

void ARGBPlayerCharacter::StartJump()
{
	bJumpInputHeld = true;

	BufferedJumpExpiresAt = GetWorld()->GetTimeSeconds() + JumpBufferDuration;
}

void ARGBPlayerCharacter::EndJump()
{
	bJumpInputHeld = false;
	StopJumping();
}

void ARGBPlayerCharacter::StartDash()
{
	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);

	URGBCharacterMovementComponent* RGBMovement = Cast<URGBCharacterMovementComponent>(GetCharacterMovement());

	if (!EnhancedInput || !RGBMovement || !MoveAction) {
		return;
	}

	const float MoveAmount = EnhancedInput->GetBoundActionValue(MoveAction).Get<float>();
	const float DashDirection = FMath::IsNearlyZero(MoveAmount) ? LastFacingDirection : (MoveAmount > 0.0f ? 1.0f : -1.0);

	if (RGBMovement->TryStartAirDash(DashDirection)) {
		LastFacingDirection = DashDirection;
		BufferedJumpExpiresAt = -1.0f;
	}
}

void ARGBPlayerCharacter::CheckJumpInput(float DeltaTime)
{
	if (BufferedJumpExpiresAt >= 0.0) {
		const double CurrentTime = GetWorld()->GetTimeSeconds();

		if (CurrentTime > BufferedJumpExpiresAt) {
			BufferedJumpExpiresAt = -1.0f;
		}
		else if (GetCharacterMovement()->IsMovingOnGround() && CanJump()) {
			BufferedJumpExpiresAt = -1.0f;
			Jump();
		}
	}

	Super::CheckJumpInput(DeltaTime);

	if (!bJumpInputHeld) {
		StopJumping();
	}
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

	if (ensureMsgf(MoveAction, TEXT("MoveAction is not assigned")))
	{
		EnhancedInput->BindActionValue(MoveAction);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARGBPlayerCharacter::Move);
	}

	if (ensureMsgf(JumpAction, TEXT("JumpAction is not assigned"))) {
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ARGBPlayerCharacter::StartJump);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ARGBPlayerCharacter::EndJump);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ARGBPlayerCharacter::EndJump);
	}

	if (ensureMsgf(DashAction, TEXT("DashAction is not assigned"))) {
		EnhancedInput->BindAction(DashAction, ETriggerEvent::Started, this, &ARGBPlayerCharacter::StartDash);
	}
}

