// Fill out your copyright notice in the Description page of Project Settings.


#include "RGBArenaControl.h"

#include "Components/TextRenderComponent.h"
#include "Components/SceneComponent.h"
#include "../Camera/RGBArenaCameraZone.h"
#include "../Platforms/RGBColorPlatform.h"

#include "../AI/RGBEnemyCharacter.h"
#include "Engine/World.h"

#include "Kismet/GameplayStatics.h"

// Sets default values
ARGBArenaControl::ARGBArenaControl()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ProgressText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ProgressText"));
	ProgressText->SetupAttachment(SceneRoot);
	ProgressText->SetRelativeLocation(FVector(0.0f, 0.0f, 400.0f));
	ProgressText->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	ProgressText->SetHorizontalAlignment(EHTA_Center);
	ProgressText->SetWorldSize(80.0f);
	ProgressText->SetTextRenderColor(FColor::White);
	ProgressText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProgressText->SetText(FText::FromString(TEXT("Arena Progress")));
}

void ARGBArenaControl::BeginArenaReset()
{
	GetWorldTimerManager().ClearTimer(InitialEvaluationTimer);

	bResetInProgress = true;
	bCompleted = false;
	MatchingPlatformCount = 0;
	CompletionPercentage = 0.0f;

	OnArenaResetStarted.Broadcast();
	OnArenaProgressChanged.Broadcast(0.0f);

	for (ARGBColorPlatform* Platform : AssignedPlatforms) {
		if (IsValid(Platform)) {
			if (URGBColorComponent* Color = Platform->GetColorComponent()) {
				Color->SetColorLocked(false);
			}
		}
	}

	for (ARGBEnemyCharacter* Enemy : AssignedEnemies) {
		if (IsValid(Enemy)) {
			Enemy->DespawnForArenaCompletion();
		}
	}

	AssignedEnemies.Reset();
	bEnemiesNeedRespawn = true;
}

void ARGBArenaControl::FinishArenaReset()
{
	if (bEnemiesNeedRespawn) {
		RespawnRecordedEnemy();
		bEnemiesNeedRespawn = false;
	}

	bResetInProgress = false;
	UpdateProgress();
}

void ARGBArenaControl::BeginPlay()
{
	Super::BeginPlay();

	for (ARGBColorPlatform* Platform : AssignedPlatforms) {
		if (!IsValid(Platform)) {
			continue;
		}

		if (URGBColorComponent* Color = Platform->GetColorComponent()) {
			Color->OnColorChanged.AddUniqueDynamic(this, &ARGBArenaControl::HandlePlatformColorChanged);
		}

		Platform->OnDestroyed.AddUniqueDynamic(this, &ARGBArenaControl::HandlePlatformDestroyed);
	}

	for (ARGBEnemyCharacter* Enemy : AssignedEnemies) {
		RecordEnemySpawn(Enemy);
	}

	InitialEvaluationTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ARGBArenaControl::FinishArenaReset);

	if (UWorld* World = GetWorld()) {
		ActorSpawnedHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &ARGBArenaControl::HandleActorSpawned));
	}
}

void ARGBArenaControl::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(InitialEvaluationTimer);

	if (ActorSpawnedHandle.IsValid()) {
		if (UWorld* World = GetWorld()) {
			World->RemoveOnActorSpawnedHandler(ActorSpawnedHandle);
		}
		ActorSpawnedHandle.Reset();
	}

	for (ARGBColorPlatform* Platform : AssignedPlatforms) {
		if (!IsValid(Platform)) {
			continue;
		}

		if (URGBColorComponent* Color = Platform->GetColorComponent()) {
			Color->OnColorChanged.RemoveDynamic(this, &ARGBArenaControl::HandlePlatformColorChanged);
		}
		Platform->OnDestroyed.RemoveDynamic(this, &ARGBArenaControl::HandlePlatformDestroyed);
	}
	Super::EndPlay(EndPlayReason);
}

void ARGBArenaControl::HandlePlatformColorChanged(ERGBColor PreviousColor, ERGBColor NewColor)
{
	if (bResetInProgress || bCompleted) {
		return;
	}

	USoundBase* ResultSound = NewColor == RequiredColor ? CorrectColorSound.Get() : WrongColorSound.Get();

	if (IsValid(ResultSound)) {
		UGameplayStatics::PlaySoundAtLocation(this, ResultSound, GetActorLocation());
	}

	UpdateProgress();
}

void ARGBArenaControl::HandlePlatformDestroyed(AActor* DestroyedActor)
{
	for (TObjectPtr<ARGBColorPlatform>& Platform : AssignedPlatforms) {
		if (Platform.Get() == DestroyedActor) {
			Platform = nullptr;
		}
	}
	UpdateProgress();
}

void ARGBArenaControl::UpdateProgress() {

	if (bResetInProgress || bCompleted) {
		return;
	}

	MatchingPlatformCount = 0;
	CompletionPercentage = 0.0f;

	TSet<ARGBColorPlatform*> SeenPlatforms;
	bool bValidSetup = AssignedPlatforms.Num() > 0;

	const bool bValidColor = RequiredColor == ERGBColor::Red || RequiredColor == ERGBColor::Green || RequiredColor == ERGBColor::Blue;
	bValidSetup = bValidSetup && bValidColor;

	for (ARGBColorPlatform* Platform : AssignedPlatforms) {
		if (!IsValid(Platform) || SeenPlatforms.Contains(Platform)) {
			bValidSetup = false;
			continue;
		}

		SeenPlatforms.Add(Platform);

		URGBColorComponent* Color = Platform->GetColorComponent();
		
		if (!IsValid(Color)) {
			bValidSetup = false;
			continue;
		}

		if (Color->GetCurrentColor() == RequiredColor) {
			++MatchingPlatformCount;
		}
	}

	if (!bValidSetup) {
		MatchingPlatformCount = 0;

		ProgressText->SetText(FText::FromString(TEXT("Arena setup error - check assigned platforms")));
		UE_LOG(LogTemp, Warning, TEXT("%s: assign at least one platform, with no missing or ") TEXT("duplicate entries, and select a valid required color."), *GetName());
		OnArenaProgressChanged.Broadcast(0.0f);
		return;
	}

	const int32 TotalPlatforms = AssignedPlatforms.Num();

	CompletionPercentage = 100.0f * MatchingPlatformCount / TotalPlatforms;

	OnArenaProgressChanged.Broadcast(GetCompletionFraction());

	const FString ColorName = StaticEnum<ERGBColor>()->GetNameStringByValue(static_cast<int64>(RequiredColor));

	ProgressText->SetText(FText::FromString(FString::Printf(TEXT("Progress: %.1f%%"), CompletionPercentage)));

	if (MatchingPlatformCount == TotalPlatforms) {
		CompleteArena();
	}
}

void ARGBArenaControl::CompleteArena()
{
	if (bCompleted || bResetInProgress) {
		return;
	}

	bCompleted = true;

	if (IsValid(ArenaCompletedSound)) {
		UGameplayStatics::PlaySoundAtLocation(this, ArenaCompletedSound, GetActorLocation());
	}

	for (ARGBColorPlatform* Platform : AssignedPlatforms) {
		if (IsValid(Platform)) {
			if (URGBColorComponent* Color = Platform->GetColorComponent()) {
				Color->SetColorLocked(true);
			}
		}
	}

	for (ARGBEnemyCharacter* Enemy : AssignedEnemies) {
		if (IsValid(Enemy)) {
			Enemy->DespawnForArenaCompletion();
		}
	}

	ProgressText->SetText(FText::FromString(TEXT("Progress: 100.0% - Completed")));
	OnArenaCompleted.Broadcast();
}

void ARGBArenaControl::HandleActorSpawned(AActor* SpawnedActor)
{
	RegisterEnemyIfInsideArena(Cast<ARGBEnemyCharacter>(SpawnedActor));
}

void ARGBArenaControl::RegisterEnemyIfInsideArena(ARGBEnemyCharacter* Enemy)
{
	if (!IsValid(Enemy) || !IsValid(ArenaCameraZone) || !ArenaCameraZone->ContainWorldLocation(Enemy->GetActorLocation())) {
		return;
	}

	if (bCompleted) {
		Enemy->DespawnForArenaCompletion();
		return;
	}

	AssignedEnemies.AddUnique(Enemy);

	if (!bRespawningEnemies) {
		RecordEnemySpawn(Enemy);
	}
}

void ARGBArenaControl::RecordEnemySpawn(ARGBEnemyCharacter* Enemy)
{
	if (!IsValid(Enemy)) {
		return;
	}

	const TSubclassOf<ARGBEnemyCharacter> EnemyClass = Enemy->GetClass();
	const FTransform SpawnTransform = Enemy->GetActorTransform();

	for (const FRGBArenaEnemySpawnRecord& ExistingRecord : EnemySpawnRecord) {
		if (ExistingRecord.EnemyClass == EnemyClass && ExistingRecord.SpawnTransform.Equals(SpawnTransform)) {
			return;
		}
	}

	FRGBArenaEnemySpawnRecord NewRecord;
	NewRecord.EnemyClass = EnemyClass;
	NewRecord.SpawnTransform = SpawnTransform;

	EnemySpawnRecord.Add(NewRecord);
}

void ARGBArenaControl::RespawnRecordedEnemy()
{
	UWorld* World = GetWorld();

	if (!IsValid(World)) {
		return;
	}

	bRespawningEnemies = true;

	for (const FRGBArenaEnemySpawnRecord& Record : EnemySpawnRecord) {
		if (!Record.EnemyClass) {
			continue;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParameters.TransformScaleMethod = ESpawnActorScaleMethod::OverrideRootScale;

		ARGBEnemyCharacter* Enemy = World->SpawnActor<ARGBEnemyCharacter>(Record.EnemyClass, Record.SpawnTransform, SpawnParameters);

		if (IsValid(Enemy)) {
			AssignedEnemies.AddUnique(Enemy);
		}
	}
	bRespawningEnemies = false;
}
