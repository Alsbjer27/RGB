#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../AI/RGBArenaEnemyTuning.h"
#include "../AI/RGBArenaEnemyCoordinatorComponent.h"
#include "../AI/RGBFlyingEnemyCharacter.h"
#include "../AI/RGBTeleporterEnemyCharacter.h"
#include "../Arena/RGBArenaControl.h"
#include "../Platforms/RGBColorPlatform.h"
#include "../Player/RGBPlayerCharacter.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRGBProgressResponseTest, "RGB.ArenaAI.ProgressResponses", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRGBProgressResponseTest::RunTest(const FString& Parameters)
{
	FRGBProgressValue Delay(8.0f, 2.0f);
	TestEqual(TEXT("Zero progress"), Delay.Evaluate(0.0f), 8.0f);
	TestEqual(TEXT("Midpoint"), Delay.Evaluate(0.5f), 5.0f);
	TestEqual(TEXT("Full progress"), Delay.Evaluate(1.0f), 2.0f);
	TestEqual(TEXT("Negative progress clamps"), Delay.Evaluate(-1.0f), 8.0f);
	TestEqual(TEXT("Excess progress clamps"), Delay.Evaluate(2.0f), 2.0f);
	Delay.ResponseCurve = NewObject<UCurveFloat>();
	Delay.ResponseCurve->FloatCurve.AddKey(0.0f, 0.0f);
	Delay.ResponseCurve->FloatCurve.AddKey(0.5f, 0.0f);
	Delay.ResponseCurve->FloatCurve.AddKey(1.0f, 1.0f);
	TestEqual(TEXT("Curve can defer escalation"), Delay.Evaluate(0.5f), 8.0f);
	TestEqual(TEXT("Curve preserves end"), Delay.Evaluate(1.0f), 2.0f);
	Delay.ResponseCurve->FloatCurve.AddKey(0.25f, 3.0f);
	TestEqual(TEXT("Curve overshoot is bounded"), Delay.Evaluate(0.25f), 2.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRGBTeleportPressureTest, "RGB.ArenaAI.TeleportPressure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRGBTeleportPressureTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Nearby points score above distant ones"), RGBEnemyTuning::ScoreTeleportDanger(100.0f, 100.0f, 900.0f, 300.0f) > RGBEnemyTuning::ScoreTeleportDanger(900.0f, 100.0f, 900.0f, 300.0f));
	TestTrue(TEXT("Equal distances do not divide by zero"), FMath::IsFinite(RGBEnemyTuning::ScoreTeleportDanger(100.0f, 100.0f, 100.0f, 0.0f)));
	TestTrue(TEXT("Horizontal range uses X only after level eligibility"), RGBEnemyTuning::IsWithinHorizontalAttackRange(FVector(0, 0, -10000), FVector(100, 0, 10000), 250.0f));
	TestFalse(TEXT("A different floor is ineligible even when X matches"), RGBEnemyTuning::IsSameCombatLevel(0.0f, 400.0f, 60.0f));
	TestTrue(TEXT("Small floor-height differences remain eligible"), RGBEnemyTuning::IsSameCombatLevel(0.0f, 30.0f, 60.0f));
	TestFalse(TEXT("Excess horizontal distance still rejects"), RGBEnemyTuning::IsWithinHorizontalAttackRange(FVector::ZeroVector, FVector(251, 0, 0), 250.0f));
	TestFalse(TEXT("Zero attack range disables attack"), RGBEnemyTuning::IsWithinHorizontalAttackRange(FVector::ZeroVector, FVector::ZeroVector, 0.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRGBSharedAttackBudgetTest, "RGB.ArenaAI.SharedAttackBudget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRGBSharedAttackBudgetTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues InitValues = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &InitValues);
	if (!TestNotNull(TEXT("Isolated test world"), World)) { return false; }
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACharacter* Player = World->SpawnActor<ACharacter>(FVector(0, 0, 1000), FRotator::ZeroRotator, Spawn);
	ARGBArenaControl* Arena = World->SpawnActor<ARGBArenaControl>(FVector(0, 0, -5000), FRotator::ZeroRotator, Spawn);
	ARGBFlyingEnemyCharacter* Flyer = World->SpawnActor<ARGBFlyingEnemyCharacter>(FVector(100, 0, 1000), FRotator::ZeroRotator, Spawn);
	ARGBFlyingEnemyCharacter* DistantFlyer = World->SpawnActor<ARGBFlyingEnemyCharacter>(FVector(600, 0, 1600), FRotator::ZeroRotator, Spawn);
	ARGBTeleporterEnemyCharacter* Teleporter = World->SpawnActor<ARGBTeleporterEnemyCharacter>(FVector(-120, 0, 1000), FRotator::ZeroRotator, Spawn);
	URGBArenaEnemyCoordinatorComponent* Coordinator = Arena ? Arena->FindComponentByClass<URGBArenaEnemyCoordinatorComponent>() : nullptr;
	if (!Player || !Flyer || !DistantFlyer || !Teleporter || !Coordinator) { AddError(TEXT("Could not create test actors")); World->DestroyWorld(false); return false; }

	FRGBEnemyArenaAttributes Attributes;
	Attributes.bEnabled = true;
	Flyer->ApplyArenaAttributes(Attributes);
	DistantFlyer->ApplyArenaAttributes(Attributes);
	Teleporter->ApplyArenaAttributes(Attributes);
	Coordinator->ConfigureArenaPolicy(0.2f, 0.0f, 1);
	Coordinator->SetPlayerTarget(Player);
	Coordinator->RegisterEnemy(Flyer, ERGBEnemyMobilityType::Flying);
	Coordinator->RegisterEnemy(DistantFlyer, ERGBEnemyMobilityType::Flying);
	Coordinator->RegisterEnemy(Teleporter, ERGBEnemyMobilityType::Teleporter);
	TestTrue(TEXT("Nearest ready flyer gets first slot"), Coordinator->TryAcquireAttackPermission(Flyer));
	TestFalse(TEXT("Teleporter cannot bypass shared limit"), Coordinator->TryAcquireAttackPermission(Teleporter));
	Coordinator->SetMaximumAttackers(3);
	TestTrue(TEXT("Raised budget includes eligible teleporter on same level"), Coordinator->TryAcquireAttackPermission(Teleporter));
	Coordinator->SetMaximumAttackers(0);
	TestTrue(TEXT("Executing flyer retains permission during budget decrease"), Coordinator->CanEnemyAttack(Flyer));
	TestTrue(TEXT("Executing teleporter retains permission during budget decrease"), Coordinator->CanEnemyAttack(Teleporter));
	TestFalse(TEXT("Zero budget admits no new attack"), Coordinator->TryAcquireAttackPermission(DistantFlyer));
	Coordinator->ReleaseAttackPermission(Flyer);
	Coordinator->ReleaseAttackPermission(Teleporter);
	Coordinator->SetPlayerTarget(Player);
	TestFalse(TEXT("Released attack cannot restart with zero budget"), Coordinator->TryAcquireAttackPermission(Flyer));
	Flyer->SetArenaAIActive(false);
	Coordinator->SetMaximumAttackers(3);
	TestFalse(TEXT("Inactive enemies cannot acquire permission"), Coordinator->TryAcquireAttackPermission(Flyer));

	// Progress must never overwrite the user's point-in-animation damage timing.
	const FFloatProperty* HitDelay = FindFProperty<FFloatProperty>(ARGBEnemyCharacter::StaticClass(), TEXT("AttackHitDelay"));
	if (TestNotNull(TEXT("Attack hit delay property"), HitDelay)) {
		const float OriginalDelay = HitDelay->GetPropertyValue_InContainer(Flyer);
		Attributes.Completion = 0.9f;
		Attributes.AttackCooldownMultiplier = 0.3f;
		Flyer->ApplyArenaAttributes(Attributes);
		TestEqual(TEXT("Arena pressure preserves hit timing"), HitDelay->GetPropertyValue_InContainer(Flyer), OriginalDelay);
	}
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRGBArenaLifecycleTest, "RGB.ArenaAI.ConfigurationAcrossResets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRGBArenaLifecycleTest::RunTest(const FString& Parameters)
{
	// This isolated world has no game mode; permit native dynamic delegates before BeginPlay.
	TGuardValue<bool> AllowTestDelegates(GAllowActorScriptExecutionInEditor, true);
	const UWorld::InitializationValues InitValues = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &InitValues);
	if (!TestNotNull(TEXT("Lifecycle world"), World)) { return false; }
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARGBArenaControl* Arena = World->SpawnActor<ARGBArenaControl>(FVector(0, 0, -5000), FRotator::ZeroRotator, Spawn);
	ARGBFlyingEnemyCharacter* First = World->SpawnActor<ARGBFlyingEnemyCharacter>(FVector(100, 0, 1000), FRotator::ZeroRotator, Spawn);
	ARGBFlyingEnemyCharacter* Second = World->SpawnActor<ARGBFlyingEnemyCharacter>(FVector(100, 0, 1000), FRotator::ZeroRotator, Spawn);
	ARGBTeleporterEnemyCharacter* Teleporter = World->SpawnActor<ARGBTeleporterEnemyCharacter>(FVector(500, 0, 1000), FRotator::ZeroRotator, Spawn);
	ARGBColorPlatform* Platform = World->SpawnActor<ARGBColorPlatform>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	if (!Arena || !First || !Second || !Teleporter || !Platform) { AddError(TEXT("Could not create lifecycle actors")); World->DestroyWorld(false); return false; }
	Arena->AssignedPlatforms.Add(Platform);
	Arena->bResetInProgress = false;
	Arena->RecordEnemySpawn(First);
	Arena->RecordEnemySpawn(Second);
	Arena->RecordEnemySpawn(Teleporter);
	TestEqual(TEXT("Identical transforms retain separate spawn records"), Arena->EnemySpawnRecord.Num(), 3);
	FNameProperty* Tag = FindFProperty<FNameProperty>(ARGBTeleporterEnemyCharacter::StaticClass(), TEXT("TeleportPointTag"));
	FBoolProperty* RandomColor = FindFProperty<FBoolProperty>(URGBColorComponent::StaticClass(), TEXT("bRandomizeInitialColor"));
	URGBColorComponent* Color = Teleporter->FindComponentByClass<URGBColorComponent>();
	if (!Tag || !RandomColor || !Color) { AddError(TEXT("Configuration properties unavailable")); World->DestroyWorld(false); return false; }
	Tag->SetPropertyValue_InContainer(Teleporter, FName(TEXT("ArenaBPoints")));
	RandomColor->SetPropertyValue_InContainer(Color, true);
	TestTrue(TEXT("Matching hit eliminates the test teleporter"), Teleporter->RecieveColorHit(Color->GetCurrentColor()));
	TestEqual(TEXT("Eliminated enemy is retained while waiting"), Arena->PendingRespawnEnemies.Num(), 1);
	TestTrue(TEXT("Eliminated instance is hidden"), Teleporter->IsHidden());
	Arena->BeginArenaReset();
	Arena->FinishArenaReset();
	TestEqual(TEXT("Reset restores exactly the configured enemy count"), Arena->AssignedEnemies.Num(), 3);
	TestTrue(TEXT("Reset reuses teleporter instance"), Arena->AssignedEnemies.Contains(Teleporter));
	TestTrue(TEXT("Reset reuses both collocated flyers"), Arena->AssignedEnemies.Contains(First) && Arena->AssignedEnemies.Contains(Second));
	TestEqual(TEXT("Instance-specific tag survives"), Tag->GetPropertyValue_InContainer(Teleporter), FName(TEXT("ArenaBPoints")));
	TestTrue(TEXT("Color randomization stays enabled"), RandomColor->GetPropertyValue_InContainer(Color));
	TestFalse(TEXT("Restored actor is visible"), Teleporter->IsHidden());
	Arena->CompleteArena();
	TestTrue(TEXT("Completion makes every configured actor inactive"), First->IsHidden() && Second->IsHidden() && Teleporter->IsHidden());
	Arena->BeginArenaReset();
	Arena->FinishArenaReset();
	TestEqual(TEXT("Reset after completion still preserves exactly three enemies"), Arena->AssignedEnemies.Num(), 3);
	TestTrue(TEXT("Completion/reset preserves original teleporter"), Arena->AssignedEnemies.Contains(Teleporter));
	TestEqual(TEXT("Tag survives completion and another reset"), Tag->GetPropertyValue_InContainer(Teleporter), FName(TEXT("ArenaBPoints")));
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRGBInactiveEnemyDamageTest, "RGB.ArenaAI.InactiveEnemyDamage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRGBInactiveEnemyDamageTest::RunTest(const FString& Parameters)
{
	TGuardValue<bool> AllowTestDelegates(GAllowActorScriptExecutionInEditor, true);
	const UWorld::InitializationValues InitValues = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &InitValues);
	if (!TestNotNull(TEXT("Damage test world"), World)) { return false; }
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ARGBPlayerCharacter* Player = World->SpawnActor<ARGBPlayerCharacter>(FVector(0, 0, 1000), FRotator::ZeroRotator, Spawn);
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	ARGBEnemyCharacter* Ground = World->SpawnActor<ARGBEnemyCharacter>(FVector(100, 0, 1000), FRotator::ZeroRotator, Spawn);
	ARGBTeleporterEnemyCharacter* Teleporter = World->SpawnActor<ARGBTeleporterEnemyCharacter>(FVector(-100, 0, 1000), FRotator::ZeroRotator, Spawn);
	if (!Player || !Controller || !Ground || !Teleporter) { AddError(TEXT("Damage test actors unavailable")); World->DestroyWorld(false); return false; }
	// No game mode initializes the controller list in this isolated world.
	World->AddController(Controller);
	Controller->Possess(Player);
	TestTrue(TEXT("Test controller possesses player"), Controller->GetPawn() == Player);
	TestEqual(TEXT("Damage test resolves the controlled player"), UGameplayStatics::GetPlayerCharacter(World, 0), static_cast<ACharacter*>(Player));
	const int32 StartingHealth = Player->GetCurrentHealth();
	FRGBEnemyArenaAttributes Attributes;
	Attributes.bEnabled = true;
	Attributes.TeleportHorizontalAttackRange = 350.0f;
	Attributes.TeleportSameLevelHeightTolerance = 60.0f;
	Teleporter->ApplyArenaAttributes(Attributes);
	TestTrue(TEXT("Same-level player within horizontal range is eligible"), Teleporter->CanBeAttackCandidate(Player));
	Player->SetActorLocation(FVector(-100, 0, 1400));
	TestFalse(TEXT("Player on a higher platform cannot trigger an attack"), Teleporter->CanBeAttackCandidate(Player));
	Teleporter->bAoEWindInProgress = true;
	Teleporter->ResolveAoEAttack();
	TestEqual(TEXT("Higher floor cannot take invisible damage"), Player->GetCurrentHealth(), StartingHealth);
	Player->SetActorLocation(FVector(-100, 0, 600));
	TestFalse(TEXT("Lower floor is also ineligible"), Teleporter->CanBeAttackCandidate(Player));
	Player->SetActorLocation(FVector(200, 0, 1000));
	TestTrue(TEXT("Horizontal trigger can extend beyond actual damage radius"), Teleporter->CanBeAttackCandidate(Player));
	Teleporter->bAoEWindInProgress = true;
	Teleporter->ResolveAoEAttack();
	TestEqual(TEXT("Outside AoE radius takes no damage despite trigger eligibility"), Player->GetCurrentHealth(), StartingHealth);
	Player->SetActorLocation(FVector(0, 0, 1000));
	Teleporter->bAoEWindInProgress = true;
	Teleporter->ResolveAoEAttack();
	TestEqual(TEXT("Active teleporter can damage player"), Player->GetCurrentHealth(), StartingHealth - 1);
	Ground->TargetPlayer = Player;
	Ground->bAttacking = true;
	Ground->TryApplyAttackDamage();
	TestEqual(TEXT("Active melee can damage player"), Player->GetCurrentHealth(), StartingHealth - 2);
	Ground->SetActorHiddenInGame(true);
	Ground->bAttacking = true;
	Ground->TryApplyAttackDamage();
	Teleporter->SetActorHiddenInGame(true);
	Teleporter->bAoEWindInProgress = true;
	Teleporter->ResolveAoEAttack();
	TestEqual(TEXT("Hidden attackers cannot damage player"), Player->GetCurrentHealth(), StartingHealth - 2);
	Ground->SetActorHiddenInGame(false);
	Ground->SetArenaAIActive(false);
	Ground->TargetPlayer = Player;
	Ground->bAttacking = true;
	Ground->TryApplyAttackDamage();
	Teleporter->SetActorHiddenInGame(false);
	Teleporter->SetArenaAIActive(false);
	Teleporter->bAoEWindInProgress = true;
	Teleporter->ResolveAoEAttack();
	TestEqual(TEXT("Inactive attackers cannot damage through stale callbacks"), Player->GetCurrentHealth(), StartingHealth - 2);
	World->DestroyWorld(false);
	return true;
}

#endif
