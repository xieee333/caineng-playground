#include "TraversalProbe.h"
#include "CanergyRuntimeComponents.h"
#include "BounceFlower.h"
#include "WeaponSpecialComponent.h"
#include "GameplayDefinitionAssets.h"
#include "ShooterCharacter.h"
#include "ShooterGameMode.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

ACanergyTraversalProbe::ACanergyTraversalProbe()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACanergyTraversalProbe::StartPhase(int32 NewPhase)
{
	Phase = NewPhase;
	Time = 0.0f;
	Character->SetLiquidDiveHeld(false);
	if (Wall.IsValid()) Wall->Destroy();
	Surface->ClearSurfaceAuthorityOnly();
	if (Phase >= 5) Surface->ApplySurfaceKindAuthorityOnly(ECanergySurfaceKind::Liquid);
	if (Phase > 0 && Phase < 4)
	{
		const ECanergySurfaceKind Kinds[] = {ECanergySurfaceKind::None, ECanergySurfaceKind::Liquid,
			ECanergySurfaceKind::Bounce, ECanergySurfaceKind::Float};
		Surface->ApplySurfaceKindAuthorityOnly(Kinds[Phase]);
	}
	const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	PhaseStart = Origin + FVector(-800.0f, 0.0f, HalfHeight + (Phase == 2 ? 160.0f : 3.0f));
	Character->SetActorLocationAndRotation(PhaseStart, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
	Character->GetCharacterMovement()->StopMovementImmediately();
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	if (Phase >= 7 && Phase <= 10)
	{
		const float Radius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
		Wall = GetWorld()->SpawnActor<AStaticMeshActor>(
			FVector(PhaseStart.X + Radius + 30.0f, PhaseStart.Y, Origin.Z + 1500.0f), FRotator::ZeroRotator);
		if (Wall.IsValid())
		{
			auto* Mesh = Wall->GetStaticMeshComponent();
			Mesh->SetMobility(EComponentMobility::Movable);
			Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
			Mesh->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
			Mesh->SetCollisionProfileName(TEXT("BlockAll"));
			Wall->SetActorScale3D(FVector(0.2f, 20.0f, 30.0f));
			auto* NewSurface = NewObject<UCanergyPaintableSurfaceComponent>(Wall.Get());
			Wall->AddInstanceComponent(NewSurface);
			NewSurface->RegisterComponent();
			NewSurface->ApplySurfaceKindAuthorityOnly(ECanergySurfaceKind::Liquid);
			WallSurface = NewSurface;
		}
	}
	if (AController* Controller = Character->GetController()) Controller->SetControlRotation(FRotator(-20.0f, 0.0f, 0.0f));
	const TCHAR* Names[] = {TEXT("普通地面前进"), TEXT("液态面加速"), TEXT("空中落到弹跳面"), TEXT("飘浮面起跳与重力恢复"), TEXT("泡泡回场与指令保护"), TEXT("Ctrl 液态潜行与退出"), TEXT("低矮通道松键保护"), TEXT("液态上墙与松键下落"), TEXT("液态墙失效保护"), TEXT("空格蹬墙脱离"), TEXT("上墙时受击回场"), TEXT("棱花盛放 · 玩家与道具弹射")};
	if (AShooterGameMode* Mode = GetWorld()->GetAuthGameMode<AShooterGameMode>())
	{
		Mode->ShowPlayerFeedback(FText::FromString(FString::Printf(TEXT("开发自动实测 %d/12 · %s"), Phase + 1, Names[Phase])), 5.0f);
	}
	if (Phase == 4)
	{
		auto* Bubble = Character->FindComponentByClass<UCanergyBubbleRespawnComponent>();
		auto* Weapon = Character->FindComponentByClass<UCanergyToyWeaponControllerComponent>();
		auto* Ability = Character->FindComponentByClass<UCanergyAbilityControllerComponent>();
		auto* Joy = Character->FindComponentByClass<UCanergyJoyScoreComponent>();
		if (Bubble && Weapon && Ability && Joy)
		{
			Weapon->StartFiring();
			JoyBeforeBubble = Joy->GetJoyScore();
			const bool bWasFiring = Weapon->IsFiring();
			Character->TakeDamage(100000.0f, FDamageEvent(), nullptr, this);
			bBubbleRulesOk = bWasFiring && Bubble->IsBubbled() && !Character->IsActorBeingDestroyed();
			bBubbleRulesOk &= !Weapon->IsFiring() && !Bubble->BeginBubbleAuthorityOnly();
			Weapon->StartFiring();
			const float Cooldown = Ability->GetSelectedAbilityCooldownRemaining();
			bBubbleRulesOk &= !Weapon->IsFiring() && !Ability->ActivateSelectedAbility()
				&& FMath::IsNearlyEqual(Cooldown, Ability->GetSelectedAbilityCooldownRemaining());
			if (auto* Special = Character->FindComponentByClass<UCanergyWeaponSpecialComponent>())
				bBloomBubbleProtected = !Special->ActivateSpecial() && Special->GetCooldownRemaining() == 0.0f;
		}
	}
}

void ACanergyTraversalProbe::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Time += DeltaSeconds;
	if (Phase == -1)
	{
		if (Time < 2.0f) return;
		Character = Cast<AShooterCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
		if (!Character.IsValid())
		{
			if (Time > 15.0f) { UE_LOG(LogTemp, Error, TEXT("CANERGY_TRAVERSAL FAIL: missing player")); Destroy(); }
			return;
		}
		OriginalTransform = Character->GetActorTransform();
		OriginalView = Character->GetControlRotation();
		BaseGravity = Character->GetCharacterMovement()->GravityScale;
		StandingHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Origin = Character->GetActorLocation() + FVector(0.0f, 0.0f, 4000.0f);
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		Floor = GetWorld()->SpawnActor<AStaticMeshActor>(Origin - FVector(0.0f, 0.0f, 10.0f), FRotator::ZeroRotator);
		if (!Cube || !Floor.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("CANERGY_TRAVERSAL FAIL: missing test floor"));
			if (Floor.IsValid()) Floor->Destroy();
			Destroy(); return;
		}
		auto* Mesh = Floor->GetStaticMeshComponent();
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetStaticMesh(Cube);
		Mesh->SetCollisionProfileName(TEXT("BlockAll"));
		Mesh->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
		Floor->SetActorScale3D(FVector(80.0f, 30.0f, 0.2f));
		auto* NewSurface = NewObject<UCanergyPaintableSurfaceComponent>(Floor.Get());
		Floor->AddInstanceComponent(NewSurface);
		NewSurface->RegisterComponent();
		Surface = NewSurface;
		Character->DisableInput(Cast<APlayerController>(Character->GetController()));
		StartPhase(0);
		return;
	}
	if (!Character.IsValid() || !Floor.IsValid() || !Surface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("CANERGY_TRAVERSAL FAIL: lost test actor"));
		if (Floor.IsValid()) Floor->Destroy();
		Destroy(); return;
	}
	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (Phase <= 1)
	{
		if (Time >= 0.5f) Character->DoMove(0.0f, 1.0f);
		if (Time >= 3.0f)
		{
			const float Distance = FVector::Dist2D(Character->GetActorLocation(), PhaseStart);
			UE_LOG(LogTemp, Display, TEXT("CANERGY_TRAVERSAL phase=%d distance=%.1f maxSpeed=%.1f velocity=%.1f yaw=%.1f surface=%d grounded=%d"),
				Phase, Distance, Movement->MaxWalkSpeed, Movement->Velocity.Size2D(), Character->GetActorRotation().Yaw,
				static_cast<int32>(Surface->GetSurfaceKind()), Movement->IsMovingOnGround());
			if (Phase == 0) NormalDistance = Distance; else LiquidDistance = Distance;
			StartPhase(Phase + 1);
		}
	}
	else if (Phase == 2)
	{
		MaximumUpSpeed = FMath::Max(MaximumUpSpeed, Movement->Velocity.Z);
		if (Time >= 3.0f) StartPhase(3);
	}
	else if (Phase == 3)
	{
		if (!bJumped && Time >= 0.5f && Movement->IsMovingOnGround())
		{
			Character->DoJumpStart(); bJumped = true;
		}
		if (Time >= 0.7f) Character->DoJumpEnd();
		const float FootHeight = Character->GetActorLocation().Z - Origin.Z
			- Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		bFloatAwayFromGround |= FootHeight > 100.0f && Movement->GravityScale < BaseGravity * 0.5f;
		bGravityRestored |= Time > 2.5f && FMath::IsNearlyEqual(Movement->GravityScale, BaseGravity);
		if (Time >= 4.0f) StartPhase(4);
	}
	else if (Phase == 4)
	{
		auto* Bubble = Character->FindComponentByClass<UCanergyBubbleRespawnComponent>();
		if (Bubble && !Bubble->IsBubbled() && BubbleReturnTime == 0.0f) BubbleReturnTime = Time;
		if (Time >= 4.0f)
		{
			auto* Joy = Character->FindComponentByClass<UCanergyJoyScoreComponent>();
			bBubbleRulesOk &= BubbleReturnTime >= 2.9f && BubbleReturnTime <= 3.5f
				&& Joy && Joy->GetJoyScore() == JoyBeforeBubble && Movement->MovementMode != MOVE_None
				&& Character->GetCapsuleComponent()->GetCollisionEnabled() != ECollisionEnabled::NoCollision;
			UE_LOG(LogTemp, Display, TEXT("CANERGY_BUBBLE %s returnSeconds=%.2f joyBefore=%d joyAfter=%d"),
				bBubbleRulesOk ? TEXT("PASS") : TEXT("FAIL"), BubbleReturnTime, JoyBeforeBubble, Joy ? Joy->GetJoyScore() : -1);
			StartPhase(5);
		}
	}
	else if (Phase == 5)
	{
		const float Height = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		if (Time < 0.5f) StandingCameraHeight = Character->GetFirstPersonCameraComponent()->GetComponentLocation().Z;
		if (Time >= 0.5f && Time < 1.8f)
		{
			Character->SetLiquidDiveHeld(true);
			Character->DoMove(0.0f, 1.0f);
			DivePeakSpeed = FMath::Max(DivePeakSpeed, Movement->Velocity.Size2D());
			bDiveEntered |= Character->IsLiquidDiving() && Character->bIsCrouched && Height < StandingHalfHeight * 0.8f;
			if (Character->bIsCrouched) DiveCameraDrop = FMath::Max(DiveCameraDrop,
				StandingCameraHeight - Character->GetFirstPersonCameraComponent()->GetComponentLocation().Z);
		}
		else if (Time >= 1.8f && Time < 2.5f)
		{
			Character->SetLiquidDiveHeld(false);
			bDiveReleased |= !Character->IsLiquidDiving() && !Character->bIsCrouched
				&& FMath::IsNearlyEqual(Height, StandingHalfHeight);
		}
		else if (Time >= 2.5f && Time < 3.2f)
		{
			Character->SetLiquidDiveHeld(true);
			bDiveReentered |= Character->IsLiquidDiving() && Character->bIsCrouched;
		}
		else if (Time >= 3.2f)
		{
			// Hold the same command while removing the painted surface underneath.
			Surface->ClearSurfaceAuthorityOnly();
			bDiveSurfaceExit |= Time > 3.5f && !Character->IsLiquidDiving() && !Character->bIsCrouched
				&& Movement->MaxWalkSpeedCrouched < DivePeakSpeed;
			if (Time >= 4.5f)
			{
				UE_LOG(LogTemp, Display, TEXT("CANERGY_DIVE entered=%d released=%d reentered=%d surfaceExit=%d peakSpeed=%.1f standingHalfHeight=%.1f"),
					bDiveEntered, bDiveReleased, bDiveReentered, bDiveSurfaceExit, DivePeakSpeed, StandingHalfHeight);
				StartPhase(6);
			}
		}
	}
	else if (Phase == 6)
	{
		if (Time >= 0.5f && Time < 1.5f) Character->SetLiquidDiveHeld(true);
		if (Time >= 1.0f && Time < 1.5f && !Ceiling.IsValid() && Character->bIsCrouched)
		{
			Ceiling = GetWorld()->SpawnActor<AStaticMeshActor>(
				FVector(PhaseStart.X, PhaseStart.Y, Origin.Z + StandingHalfHeight * 1.6f + 10.0f), FRotator::ZeroRotator);
			if (Ceiling.IsValid())
			{
				auto* Mesh = Ceiling->GetStaticMeshComponent();
				Mesh->SetMobility(EComponentMobility::Movable);
				Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
				Mesh->SetCollisionProfileName(TEXT("BlockAll"));
				Ceiling->SetActorScale3D(FVector(6.0f, 6.0f, 0.2f));
			}
		}
		if (Time >= 1.5f) Character->SetLiquidDiveHeld(false);
		if (Time >= 1.8f && Time < 2.5f)
			bCeilingCrouchSafe |= Ceiling.IsValid() && Character->bIsCrouched && !Character->IsLiquidDiving()
				&& Movement->MaxWalkSpeedCrouched < 600.0f;
		if (Time >= 2.5f && Ceiling.IsValid()) Ceiling->Destroy();
		if (Time >= 3.0f)
			bCeilingExitRestored |= !Character->bIsCrouched
				&& FMath::IsNearlyEqual(Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), StandingHalfHeight);
		if (Time >= 3.5f)
		{
			UE_LOG(LogTemp, Display, TEXT("CANERGY_DIVE ceilingSafe=%d ceilingExit=%d cameraDrop=%.1f"),
				bCeilingCrouchSafe, bCeilingExitRestored, DiveCameraDrop);
			StartPhase(7);
		}
	}
	else if (Phase >= 7 && Phase <= 10)
	{
		const int32 Index = Phase - 7;
		bWallEntered[Index] |= Character->IsWallTraversing();
		WallHeights[Index] = FMath::Max(WallHeights[Index], Character->GetActorLocation().Z - PhaseStart.Z);
		if (Time >= 0.5f && Time < 1.5f)
		{
			Character->SetLiquidDiveHeld(true);
			Character->DoMove(0.0f, 1.0f);
		}
		if (Time >= 1.5f)
		{
			if (Phase == 7)
			{
				Character->SetLiquidDiveHeld(false);
				bWallReleaseFalling |= !Character->IsWallTraversing() && Movement->IsFalling();
			}
			else if (Phase == 8)
			{
				if (WallSurface.IsValid()) WallSurface->ClearSurfaceAuthorityOnly();
				bWallSurfaceLossFalling |= !Character->IsWallTraversing() && Movement->IsFalling();
			}
			else if (Phase == 9)
			{
				if (Character->IsWallTraversing()) Character->DoJumpStart();
				bWallJumpAway |= !Character->IsWallTraversing() && Movement->IsFalling()
					&& Movement->Velocity.X < -300.0f && Movement->Velocity.Z > 300.0f;
			}
			else if (!bWallBubbleStarted)
			{
				bWallBubbleStarted = true;
				Character->TakeDamage(100000.0f, FDamageEvent(), nullptr, this);
				bWallBubbleCancelled = !Character->IsWallTraversing() && Character->GetCanergyBubbleRespawn()->IsBubbled()
					&& Movement->MovementMode != MOVE_Flying;
			}
			if (Phase == 10 && Time > 4.7f)
				bWallBubbleReturned |= !Character->IsDead() && !Character->GetCanergyBubbleRespawn()->IsBubbled()
					&& !Character->IsWallTraversing() && Movement->MovementMode != MOVE_None && Movement->MovementMode != MOVE_Flying;
		}
		if (Time >= (Phase == 10 ? 5.5f : 3.0f))
		{
			UE_LOG(LogTemp, Display, TEXT("CANERGY_WALL phase=%d entered=%d rise=%.1f release=%d surfaceLoss=%d jump=%d bubbleCancel=%d bubbleReturn=%d"),
				Phase, bWallEntered[Index], WallHeights[Index], bWallReleaseFalling, bWallSurfaceLossFalling,
				bWallJumpAway, bWallBubbleCancelled, bWallBubbleReturned);
			StartPhase(Phase + 1);
		}
	}
	else if (Phase == 11)
	{
		if (Time >= 0.5f && !bBloomAttempted)
		{
			bBloomAttempted = true;
			BloomProp = GetWorld()->SpawnActor<AStaticMeshActor>(
				FVector(PhaseStart.X, PhaseStart.Y + 100.0f, Origin.Z + 40.0f), FRotator::ZeroRotator);
			if (BloomProp.IsValid())
			{
				auto* Mesh = BloomProp->GetStaticMeshComponent();
				Mesh->SetMobility(EComponentMobility::Movable);
				Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
				BloomProp->SetActorScale3D(FVector(0.4f));
				Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
				Mesh->SetSimulatePhysics(true);
				Mesh->SetMassOverrideInKg(NAME_None, 10.0f, true);
			}
			if (auto* Special = Character->FindComponentByClass<UCanergyWeaponSpecialComponent>())
			{
				bBloomSpawned = Special->ActivateSpecial();
				TestFlower = Special->GetLastFlower();
				const float Cooldown = Special->GetCooldownRemaining();
				bBloomCooldownProtected = Cooldown > 7.0f && !Special->ActivateSpecial()
					&& FMath::IsNearlyEqual(Cooldown, Special->GetCooldownRemaining());
			}
		}
		BloomCharacterUpSpeed = FMath::Max(BloomCharacterUpSpeed, Movement->Velocity.Z);
		if (BloomProp.IsValid()) BloomPropUpSpeed = FMath::Max(BloomPropUpSpeed,
			BloomProp->GetStaticMeshComponent()->GetPhysicsLinearVelocity().Z);
		if (Time >= 9.5f)
		{
			bBloomExpired = bBloomSpawned && !TestFlower.IsValid();
			UE_LOG(LogTemp, Display, TEXT("CANERGY_BLOOM spawned=%d cooldown=%d bubbleGuard=%d expired=%d playerVz=%.1f propVz=%.1f"),
				bBloomSpawned, bBloomCooldownProtected, bBloomBubbleProtected, bBloomExpired, BloomCharacterUpSpeed, BloomPropUpSpeed);
			Finish();
		}
	}
}

void ACanergyTraversalProbe::Finish()
{
	bool bWallPass = bWallReleaseFalling && bWallSurfaceLossFalling && bWallJumpAway
		&& bWallBubbleCancelled && bWallBubbleReturned;
	for (int32 Index = 0; Index < 4; ++Index) bWallPass &= bWallEntered[Index] && WallHeights[Index] > 250.0f;
	const bool bPass = NormalDistance > 300.0f && LiquidDistance > NormalDistance * 1.25f
		&& MaximumUpSpeed > 700.0f && bFloatAwayFromGround && bGravityRestored && bBubbleRulesOk
		&& bDiveEntered && bDiveReleased && bDiveReentered && bDiveSurfaceExit && DivePeakSpeed > 1050.0f
		&& bCeilingCrouchSafe && bCeilingExitRestored && DiveCameraDrop > 25.0f && bWallPass
		&& bBloomSpawned && bBloomCooldownProtected && bBloomBubbleProtected && bBloomExpired
		&& BloomCharacterUpSpeed > 700.0f && BloomPropUpSpeed > 500.0f;
	UE_LOG(LogTemp, Display, TEXT("CANERGY_TRAVERSAL %s normal=%.1f liquid=%.1f bounceVz=%.1f floatAway=%d gravityRestored=%d"),
		bPass ? TEXT("PASS") : TEXT("FAIL"), NormalDistance, LiquidDistance, MaximumUpSpeed, bFloatAwayFromGround, bGravityRestored);
	if (AShooterGameMode* Mode = GetWorld()->GetAuthGameMode<AShooterGameMode>())
	{
		Mode->ShowPlayerFeedback(FText::FromString(bPass ? TEXT("开发移动实测通过 · 已返回游乐场") : TEXT("开发移动实测发现问题 · 结果已记录")), 10.0f);
	}
	Destroy();
}

void ACanergyTraversalProbe::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		if (Character.IsValid() && Phase >= 0)
		{
			Character->DoJumpEnd();
			Character->SetLiquidDiveHeld(false);
			Character->EnableInput(Cast<APlayerController>(Character->GetController()));
			Character->GetCharacterMovement()->StopMovementImmediately();
			Character->SetActorTransform(OriginalTransform, false, nullptr, ETeleportType::TeleportPhysics);
			if (AController* Controller = Character->GetController()) Controller->SetControlRotation(OriginalView);
		}
		if (Floor.IsValid()) Floor->Destroy();
		if (Ceiling.IsValid()) Ceiling->Destroy();
		if (Wall.IsValid()) Wall->Destroy();
		if (BloomProp.IsValid()) BloomProp->Destroy();
		if (TestFlower.IsValid()) TestFlower->Destroy();
	}
	Super::EndPlay(EndPlayReason);
}
