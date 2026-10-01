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
#include "../Platforms/RGBColorPlatform.h"

#include "../Combat/RGBProjectile.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"

#include "../Platforms/RGBJumpPad.h"

#include "../Progression/RGBAbilityUnlockSubsytem.h"



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

	Movement->JumpZVelocity = 4000.0f;
	Movement->GravityScale = 3.0f;
	Movement->AirControl = 0.55f;

	JumpMaxHoldTime = 0.2f;
	JumpMaxCount = 1;

	Movement->bApplyGravityWhileJumping = true;
	Movement->bUseFlatBaseForFloorChecks = true;

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

bool ARGBPlayerCharacter::RecieveDamage(int32 DamageAmount)
{
	if (DamageAmount <= 0 || CurrentHealth <= 0) {
		return false;
	}

	const int32 PreviousHealth = CurrentHealth;

	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, 0, FMath::Max(MaxHealth, 1));

	if (CurrentHealth == PreviousHealth) {
		return false;
	}

	OnHealthChanged.Broadcast(CurrentHealth, FMath::Max(MaxHealth, 1));

	if (CurrentHealth == 0) {
		ARGBGameMode* GameMode = GetWorld()->GetAuthGameMode<ARGBGameMode>();
		AController* PlayerController = Controller;

		if (!IsValid(GameMode) || !IsValid(PlayerController)) {
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Player death could not restart the mechanics test."));
			return true;
		}

		if (!GameMode->RestartMechanicsTest(PlayerController)) {
			UE_LOG(
				LogTemp,
				Error,
				TEXT("Player death restart failed."));
		}
		return true;
	}

	return true;
}

bool ARGBPlayerCharacter::GrantAbility(ERGBPlayerAbility Ability)
{
	UGameInstance* GameInstance = GetGameInstance();

	if (!IsValid(GameInstance)) {
		return false;
	}

	URGBAbilityUnlockSubsytem* UnlockSubsystem = GameInstance->GetSubsystem<URGBAbilityUnlockSubsytem>();

	if (!IsValid(UnlockSubsystem) || !UnlockSubsystem->UnlockAbility(Ability)) {
		return false;
	}

	switch (Ability) {
	case ERGBPlayerAbility::RedWeapon:
		SelectedWeaponColor = ERGBColor::Red;
		ApplyWeaponColorMaterial();
		OnWeaponColorChanged.Broadcast(SelectedWeaponColor);
		break;

	case ERGBPlayerAbility::GreenWeapon:
		SelectedWeaponColor = ERGBColor::Green;
		ApplyWeaponColorMaterial();
		OnWeaponColorChanged.Broadcast(SelectedWeaponColor);
		break;

	case ERGBPlayerAbility::BlueWeapon:
		SelectedWeaponColor = ERGBColor::Blue;
		ApplyWeaponColorMaterial();
		OnWeaponColorChanged.Broadcast(SelectedWeaponColor);
		break;

	case ERGBPlayerAbility::Dash:
		break;
	}

	return true;
}

// Called when the game starts or when spawned
void ARGBPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = FMath::Max(MaxHealth, 1);
	ApplyWeaponColorMaterial();
}

void ARGBPlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (ARGBJumpPad* JumpPad = Cast<ARGBJumpPad>(Hit.GetActor())) {
		const float LaunchSpeed = JumpPad->GetLaunchSpeed();

		if (!FMath::IsFinite(LaunchSpeed) || LaunchSpeed <= 0.0f) {
			UE_LOG(LogTemp, Warning, TEXT("%s requires a positive launch speed."), *JumpPad->GetName());
			return;
		}

		if (Hit.ImpactNormal.Z < 0.5f) {
			return;
		}

		bJumpInputHeld = false;
		BufferedJumpExpiresAt = -1.0f;
		JumpSourcePlatform.Reset();
		StopJumping();

		LaunchCharacter(FVector(0.0f, 0.0f, LaunchSpeed), false, true);

		return;
	}

	if (ARGBColorPlatform* Platform = Cast<ARGBColorPlatform>(Hit.GetActor())) {
		if (URGBColorComponent* Color = Platform->GetColorComponent()) {
			Color->AdvanceColor();
		}
	}
}

void ARGBPlayerCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	if (ARGBColorPlatform* Platform = JumpSourcePlatform.Get()) {
		Platform->HandlePlayerJumpOff();
	}
}

void ARGBPlayerCharacter::Move(const FInputActionValue& Value)
{
	const float MoveAmount = FMath::Clamp(Value.Get<float>(), -1.0f, 1.0f);

	if (!FMath::IsNearlyZero(MoveAmount)) {
		LastFacingDirection = MoveAmount > 0.0f ? 1.0 : -1.0f;

		const float FacingYaw = MoveAmount > 0.0f ? 180.0f : 0.0f;
		GetMesh()->SetRelativeRotation(FRotator(0.0f, FacingYaw, 0.0f));
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
	if (!IsAbilityUnlocked(ERGBPlayerAbility::Dash)) {
		return;
	}

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

	JumpSourcePlatform.Reset();

	UCharacterMovementComponent* Movement = GetCharacterMovement();

	if (Movement->IsMovingOnGround()) {
		JumpSourcePlatform = Cast<ARGBColorPlatform>(Movement->CurrentFloor.HitResult.GetActor());
	}

	Super::CheckJumpInput(DeltaTime);

	JumpSourcePlatform.Reset();

	if (!bJumpInputHeld) {
		StopJumping();
	}
}

void ARGBPlayerCharacter::Fire()
{
	if (!IsWeaponColorUnlocked(SelectedWeaponColor)) {
		return;
	}

	if (!ensureMsgf(ProjectileClass, TEXT("Assign ProjectileClass in BP_RGBPlayerCharacter"))) {
		return;
	}

	const FVector Direction = LastFacingDirection >= 0.0f ? FVector::ForwardVector : -FVector::ForwardVector;

	const FVector Start = GetActorLocation();
	const FVector SpawnLocation = Start + Direction * 75.0f;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FHitResult Hit;

	const bool bBlocked = GetWorld()->SweepSingleByChannel(Hit, Start, SpawnLocation, FQuat::Identity, ECC_WorldDynamic, FCollisionShape::MakeSphere(10.0f), QueryParams);

	if (bBlocked) {
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("FIRE BLOCKED: Player=%s | Facing=%.1f | Hit=%s"),
			*GetName(),
			LastFacingDirection,
			*GetNameSafe(Hit.GetActor()));

		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ARGBProjectile* Projectile = GetWorld()->SpawnActor<ARGBProjectile>(ProjectileClass, SpawnLocation, Direction.Rotation(), SpawnParams);

	if (IsValid(Projectile)) {
		Projectile->InitializeColor(SelectedWeaponColor);
	}
	else {
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("FIRE SPAWN FAILED: Player=%s | Facing=%.1f"),
			*GetName(),
			LastFacingDirection);
	}
}

void ARGBPlayerCharacter::NextWeaponColor()
{
	const ERGBColor Colors[] = { ERGBColor::Red, ERGBColor::Green, ERGBColor::Blue };
	int32 CurrentIndex = 0;

	for (int32 Index = 0; Index < 3; ++Index) {
		if (Colors[Index] == SelectedWeaponColor) {
			CurrentIndex = Index;
			break;
		}
	}

	for (int32 Offset = 1; Offset <= 3; ++Offset) {
		const ERGBColor Candidate = Colors[(CurrentIndex + Offset) % 3];

		if (IsWeaponColorUnlocked(Candidate)) {
			SetSelectedWeaponColor(Candidate);
			return;
		}
	}
}

void ARGBPlayerCharacter::PreviousWeaponColor()
{
	const ERGBColor Colors[] = { ERGBColor::Red, ERGBColor::Green, ERGBColor::Blue };
	int32 CurrentIndex = 0;

	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (Colors[Index] == SelectedWeaponColor)
		{
			CurrentIndex = Index;
			break;
		}
	}

	for (int32 Offset = 1; Offset <= 3; ++Offset)
	{
		const int32 CandidateIndex = (CurrentIndex - Offset + 3) % 3;

		const ERGBColor Candidate = Colors[CandidateIndex];

		if (IsWeaponColorUnlocked(Candidate))
		{
			SetSelectedWeaponColor(Candidate);
			return;
		}
	}
}

void ARGBPlayerCharacter::SetSelectedWeaponColor(ERGBColor NewColor)
{
	const bool bValidColor = NewColor == ERGBColor::Red || NewColor == ERGBColor::Green || NewColor == ERGBColor::Blue;

	if (!ensureMsgf(bValidColor, TEXT("Invalid weapon color"))) {
		return;
	}

	if (SelectedWeaponColor == NewColor) {
		return;
	}

	SelectedWeaponColor = NewColor;
	ApplyWeaponColorMaterial();
	OnWeaponColorChanged.Broadcast(SelectedWeaponColor);
}

void ARGBPlayerCharacter::ApplyWeaponColorMaterial()
{
	UMaterialInterface* SelectedMaterial = nullptr;

	switch (SelectedWeaponColor) {
	case ERGBColor::Red:
		SelectedMaterial = RedWeaponMaterial;
		break;

	case ERGBColor::Green:
		SelectedMaterial = GreenWeaponMaterial;
		break;

	case ERGBColor::Blue:
		SelectedMaterial = BlueWeaponMaterial;
		break;
	}

	if (!ensureMsgf(
		SelectedMaterial,
		TEXT("Weapon material is not assigned for the selected color on %s"),
		*GetName()))
	{
		return;
	}

	GetMesh()->SetMaterial(2, SelectedMaterial);
}

bool ARGBPlayerCharacter::IsAbilityUnlocked(ERGBPlayerAbility Ability) const
{
	const UGameInstance* GameInstance = GetGameInstance();

	if (!IsValid(GameInstance)) {
		return false;
	}

	const URGBAbilityUnlockSubsytem* UnlockSubsystem = GameInstance->GetSubsystem<URGBAbilityUnlockSubsytem>();

	return IsValid(UnlockSubsystem) && UnlockSubsystem->IsAbilityUnlocked(Ability);

	return false;
}

bool ARGBPlayerCharacter::IsWeaponColorUnlocked(ERGBColor Color) const
{
	switch(Color) {
	case ERGBColor::Red:
		return IsAbilityUnlocked(ERGBPlayerAbility::RedWeapon);

	case ERGBColor::Green:
		return IsAbilityUnlocked(ERGBPlayerAbility::GreenWeapon);

	case ERGBColor::Blue:
		return IsAbilityUnlocked(ERGBPlayerAbility::BlueWeapon);
	
	default:
		return false;
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

	if (ensureMsgf(FireAction, TEXT("FireAction is not assigned")))
	{
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &ARGBPlayerCharacter::Fire);
	}

	if (ensureMsgf(NextWeaponColorAction, TEXT("NextWeaponColorAction is not assigned.")))
	{
		EnhancedInput->BindAction(NextWeaponColorAction, ETriggerEvent::Started, this, &ARGBPlayerCharacter::NextWeaponColor);
	}

	if (ensureMsgf(PreviousWeaponColorAction, TEXT("PreviousWeaponColorAction is not assigned.")))
	{
		EnhancedInput->BindAction(PreviousWeaponColorAction, ETriggerEvent::Started, this, &ARGBPlayerCharacter::PreviousWeaponColor);
	}
}

