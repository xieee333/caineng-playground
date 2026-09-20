// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "ShooterGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Variant_Shooter/Gameplay/CanergyRuntimeComponents.h"
#include "Gameplay/WallTraversalComponent.h"
#include "Gameplay/WeaponSpecialComponent.h"
#include "Gameplay/PhaseAbilityComponent.h"
#include "Gameplay/CarryPrototype.h"

namespace
{
	const FName SurfaceTags[] = { FName("Surface_Cyan"), FName("Surface_Orange"), FName("Surface_Magenta") };

	int32 GetSurfaceMode(const AActor* Actor)
	{
		if (!Actor) return INDEX_NONE;
		if (const UCanergyPaintableSurfaceComponent* Surface = Actor->FindComponentByClass<UCanergyPaintableSurfaceComponent>())
		{
			switch (Surface->GetSurfaceKind())
			{
			case ECanergySurfaceKind::Liquid: return 0;
			case ECanergySurfaceKind::Bounce: return 1;
			case ECanergySurfaceKind::Float: return 2;
			default: return INDEX_NONE;
			}
		}
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(SurfaceTags); ++Index)
		{
			if (Actor->ActorHasTag(SurfaceTags[Index])) return Index;
		}
		return INDEX_NONE;
	}

}

AShooterCharacter::AShooterCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	// create the noise emitter component
	PawnNoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("Pawn Noise Emitter"));
	CanergyInteraction = CreateDefaultSubobject<UCanergyInteractionComponent>(TEXT("Canergy Interaction"));
	CanergyJoyScore = CreateDefaultSubobject<UCanergyJoyScoreComponent>(TEXT("Canergy Joy Score"));
	CanergyBubbleRespawn = CreateDefaultSubobject<UCanergyBubbleRespawnComponent>(TEXT("Canergy Bubble Respawn"));
	CanergyCharacterProfile = CreateDefaultSubobject<UCanergyCharacterProfileComponent>(TEXT("Canergy Character Profile"));
	CanergyToyWeaponController = CreateDefaultSubobject<UCanergyToyWeaponControllerComponent>(TEXT("Canergy Toy Weapon Controller"));
	CanergyAbilityController = CreateDefaultSubobject<UCanergyAbilityControllerComponent>(TEXT("Canergy Ability Controller"));
	CanergyWallTraversal = CreateDefaultSubobject<UCanergyWallTraversalComponent>(TEXT("Canergy Wall Traversal"));
	CanergyWeaponSpecial = CreateDefaultSubobject<UCanergyWeaponSpecialComponent>(TEXT("Canergy Weapon Special"));
	CanergyPhaseAbility = CreateDefaultSubobject<UCanergyPhaseAbilityComponent>(TEXT("Canergy Phase Ability"));

	// configure movement
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
	CanergyAbilityController->OnAbilityFeedback.AddDynamic(this, &AShooterCharacter::HandleCanergyAbilityFeedback);
	CanergyToyWeaponController->OnWeaponFeedback.AddDynamic(this, &AShooterCharacter::HandleCanergyWeaponFeedback);
	CanergyBubbleRespawn->OnBubbleStateChanged.AddDynamic(this, &AShooterCharacter::HandleBubbleStateChanged);
	BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	BaseCrouchSpeed = GetCharacterMovement()->MaxWalkSpeedCrouched;
	StandingFirstPersonRelativeLocation = GetFirstPersonMesh()->GetRelativeLocation();
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
	BaseGravityScale = GetCharacterMovement()->GravityScale;
	BaseGroundFriction = GetCharacterMovement()->GroundFriction;
	BaseBrakingDeceleration = GetCharacterMovement()->BrakingDecelerationWalking;
	BaseAirControl = GetCharacterMovement()->AirControl;

	// reset HP to max
	CurrentHP = MaxHP;

	// update the HUD
	OnDamaged.Broadcast(1.0f);
}

void AShooterCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Opt-in two-process verification: exercises the same client-to-server RPC as the G key.
	if (!HasAuthority() && IsLocallyControlled()
		&& FParse::Param(FCommandLine::Get(), TEXT("CanergyCarryClientProbe"))
		&& CarryClientProbeStage < 2)
	{
		CarryClientProbeElapsed += DeltaSeconds;
		const float TriggerAt = CarryClientProbeStage == 0 ? 12.0f : 14.0f;
		if (CarryClientProbeElapsed >= TriggerAt)
		{
			ServerInteractCarryCore();
			UE_LOG(LogTemp, Display, TEXT("CANERGY_CARRY_CLIENT_RPC stage=%d sent"), CarryClientProbeStage + 1);
			++CarryClientProbeStage;
		}
	}

	if (!IsDead())
	{
		UpdateTraversalSurface(DeltaSeconds);
		UpdateLiquidDive();
		UpdateMovementTuning();
	}
}

void AShooterCharacter::UpdateTraversalSurface(float DeltaSeconds)
{
	if (CanergyBubbleRespawn && CanergyBubbleRespawn->IsBubbled())
	{
		CainengGameRules::AdvanceTraversalContact(TraversalContactState, false, false, false, DeltaSeconds, true);
		CurrentTraversalMode = -1;
		return;
	}
	int32 NewMode = -1;
	FHitResult Hit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CainengSurfaceProbe), false, this);
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector TraceStart = GetActorLocation() - FVector::UpVector * (HalfHeight - 5.0f);
	const FVector TraceEnd = TraceStart - FVector::UpVector * 80.0f;

	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		if (AActor* SurfaceActor = Hit.GetActor())
		{
			if (const UCanergyPaintableSurfaceComponent* Surface = SurfaceActor->FindComponentByClass<UCanergyPaintableSurfaceComponent>())
			{
				switch (Surface->GetSurfaceKind())
				{
				case ECanergySurfaceKind::Liquid: NewMode = 0; break;
				case ECanergySurfaceKind::Bounce: NewMode = 1; break;
				case ECanergySurfaceKind::Float: NewMode = 2; break;
				case ECanergySurfaceKind::Sticky: NewMode = 3; break;
				case ECanergySurfaceKind::Mirror: NewMode = 4; break;
				case ECanergySurfaceKind::Conductive: NewMode = 5; break;
				default: break;
				}
			}
			else
			{
				NewMode = GetSurfaceMode(SurfaceActor); // Transitional compatibility for old map-authored tags.
			}
		}
	}

	const bool bShouldBounce = CainengGameRules::AdvanceTraversalContact(TraversalContactState,
		GetCharacterMovement()->IsMovingOnGround(), NewMode == 1, NewMode == 2, DeltaSeconds);
	if (bShouldBounce)
	{
		LaunchCharacter(FVector::UpVector * 850.0f, false, true);
		if (CanergyJoyScore) CanergyJoyScore->AwardJoyAuthorityOnly(ECanergyJoyAction::BounceChain);
		ShowPrototypeFeedback(FText::FromString(TEXT("弹跳彩能 · 腾空")), 1.5f);
	}
	if (NewMode == 5 && CurrentTraversalMode != 5 && GetWorld()->GetTimeSeconds() >= NextConductiveJoyTime)
	{
		NextConductiveJoyTime = GetWorld()->GetTimeSeconds() + 2.5f;
		if (CanergyJoyScore) CanergyJoyScore->AwardJoyAuthorityOnly(ECanergyJoyAction::EnvironmentalChain);
	}

	CurrentTraversalMode = NewMode;
}

void AShooterCharacter::UpdateMovementTuning()
{
	float SurfaceSpeedMultiplier = 1.0f;
	if (CurrentTraversalMode == 0)
	{
		SurfaceSpeedMultiplier = LiquidSpeedMultiplier;
	}
	else if (CurrentTraversalMode == 1)
	{
		SurfaceSpeedMultiplier = 1.12f;
	}
	else if (CurrentTraversalMode == 2)
	{
		SurfaceSpeedMultiplier = 0.82f;
	}
	else if (CurrentTraversalMode == 3)
	{
		SurfaceSpeedMultiplier = 0.56f;
	}
	else if (CurrentTraversalMode == 4)
	{
		SurfaceSpeedMultiplier = 1.0f;
	}
	else if (CurrentTraversalMode == 5)
	{
		SurfaceSpeedMultiplier = 1.22f;
	}

	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * SurfaceSpeedMultiplier * (bSprintHeld ? SprintMultiplier : 1.0f);
	// Crouched movement uses a separate cap; sprint does not stack with diving.
	GetCharacterMovement()->MaxWalkSpeedCrouched = bLiquidDiving
		? BaseWalkSpeed * LiquidDiveSpeedMultiplier : BaseCrouchSpeed;
	GetCharacterMovement()->GroundFriction = CurrentTraversalMode == 3 ? BaseGroundFriction * 1.9f : BaseGroundFriction;
	GetCharacterMovement()->BrakingDecelerationWalking = CurrentTraversalMode == 3 ? BaseBrakingDeceleration * 1.4f : BaseBrakingDeceleration;
	GetCharacterMovement()->AirControl = CurrentTraversalMode == 4 ? FMath::Max(BaseAirControl, 0.85f) : BaseAirControl;
	GetCharacterMovement()->GravityScale = (TraversalContactState.FloatSecondsRemaining > 0.0f ? FloatGravityScale : BaseGravityScale)
		* CarnivalGravityMultiplier;
}

void AShooterCharacter::SetCarnivalGravityMultiplier(float NewMultiplier)
{
	CarnivalGravityMultiplier = FMath::Clamp(NewMultiplier, 0.2f, 1.5f);
	UpdateMovementTuning();
}

void AShooterCharacter::ShowPrototypeFeedback(const FText& Message, float Duration)
{
	if (!IsPlayerControlled()) return;
	if (AShooterGameMode* GameMode = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GameMode->ShowPlayerFeedback(Message, Duration);
	}
}

void AShooterCharacter::HandleCanergyAbilityFeedback(FText Message)
{
	if (IsLocallyControlled())
	{
		ShowPrototypeFeedback(Message, 1.8f);
	}
}

void AShooterCharacter::HandleCanergyWeaponFeedback(FText Message)
{
	if (IsLocallyControlled())
	{
		ShowPrototypeFeedback(Message, 1.35f);
	}
}

void AShooterCharacter::HandleBubbleStateChanged()
{
	TraversalContactState = {};
	CurrentTraversalMode = -1;
	bSprintHeld = false;
	SetLiquidDiveHeld(false);
	if (!CanergyBubbleRespawn->IsBubbled()) CurrentHP = MaxHP;
	if (IsLocallyControlled())
	{
		ShowPrototypeFeedback(FText::FromString(CanergyBubbleRespawn->IsBubbled()
			? TEXT("泡泡回场中 · 约 3 秒后返回，欢乐值保留")
			: TEXT("已回到游乐场 · 继续喷绘、跑跳和互动")), 3.0f);
	}
}

void AShooterCharacter::PaintSurfaceAtAim()
{
	const FVector TraceStart = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector TraceEnd = TraceStart + GetFirstPersonCameraComponent()->GetForwardVector() * MaxAimDistance;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CainengPrismPaint), false, this);
	FHitResult Hit;
	bool bHasHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	AActor* HitActor = bHasHit ? Hit.GetActor() : nullptr;
	if ((!HitActor || !HitActor->ActorHasTag(FName("Relay_Target"))) && RelayAimAssistRadius > 0.0f)
	{
		FHitResult AssistedHit;
		if (GetWorld()->SweepSingleByChannel(AssistedHit, TraceStart, TraceEnd, FQuat::Identity, ECC_Visibility,
			FCollisionShape::MakeSphere(RelayAimAssistRadius), QueryParams))
		{
			AActor* AssistedActor = AssistedHit.GetActor();
			if (AssistedActor && AssistedActor->ActorHasTag(FName("Relay_Target")))
			{
				Hit = AssistedHit;
				HitActor = AssistedActor;
				bHasHit = true;
			}
		}
	}

	if (!bHasHit)
	{
		ShowPrototypeFeedback(FText::FromString(TEXT("未命中 · 调整准星或靠近目标")), 1.0f);
		return;
	}

	if (!HitActor)
	{
		return;
	}

	if (HitActor->ActorHasTag(FName("Relay_Target")))
	{
		if (AShooterGameMode* GameMode = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
		{
			GameMode->RegisterRelayHit(HitActor, SelectedPaintMode);
		}
	}

	UCanergyPaintableSurfaceComponent* Surface = HitActor->FindComponentByClass<UCanergyPaintableSurfaceComponent>();
	if (!Surface && !HitActor->ActorHasTag(FName("Paintable")))
	{
		if (!HitActor->ActorHasTag(FName("Relay_Target")))
		{
			ShowPrototypeFeedback(FText::FromString(TEXT("这里不能喷绘 · 瞄准彩色材质或中继球")), 1.2f);
		}
		return;
	}
	if (!Surface && HitActor->HasAuthority())
	{
		Surface = NewObject<UCanergyPaintableSurfaceComponent>(HitActor);
		HitActor->AddInstanceComponent(Surface);
		Surface->RegisterComponent();
	}
	if (!Surface)
	{
		ShowPrototypeFeedback(FText::FromString(TEXT("正在连接喷绘表面…请重试")), 0.8f);
		return;
	}

	static const TCHAR* SurfaceNames[] = { TEXT("青液态"), TEXT("橙弹跳"), TEXT("洋红飘浮") };
	static const ECanergySurfaceKind SurfaceKinds[] = { ECanergySurfaceKind::Liquid, ECanergySurfaceKind::Bounce, ECanergySurfaceKind::Float };
	const bool bWasUnpainted = !Surface->IsPainted();
	if (!Surface->ApplySurfaceKindAuthorityOnly(SurfaceKinds[SelectedPaintMode]))
	{
		ShowPrototypeFeedback(FText::FromString(TEXT("表面已经是这种彩能 · 换一种彩能试试")), 1.1f);
		return;
	}
	if (bWasUnpainted && CanergyJoyScore)
	{
		CanergyJoyScore->AwardJoyAuthorityOnly(ECanergyJoyAction::SurfaceTrail);
	}
	ShowPrototypeFeedback(FText::FromString(FString::Printf(TEXT("喷绘：%s · 地形效果已改变"), SurfaceNames[SelectedPaintMode])), 1.4f);
}

void AShooterCharacter::CyclePaintMode()
{
	SelectedPaintMode = (SelectedPaintMode + 1) % 3;
	if (CanergyToyWeaponController)
	{
		const FText SurfaceName = CanergyToyWeaponController->CyclePaintSurface();
		ShowPrototypeFeedback(FText::Format(FText::FromString(TEXT("喷绘模式：{0} · 左键喷绘")), SurfaceName), 2.0f);
		return;
	}
	static const TCHAR* SurfaceNames[] = { TEXT("青液态"), TEXT("橙弹跳"), TEXT("洋红飘浮") };
	ShowPrototypeFeedback(FText::FromString(FString::Printf(TEXT("喷绘模式：%s · 左键喷绘"), SurfaceNames[SelectedPaintMode])), 2.0f);
}

void AShooterCharacter::CycleSelectedAbility()
{
	if (CanergyAbilityController) CanergyAbilityController->CycleAbility();
}

void AShooterCharacter::ActivateSelectedAbility()
{
	if (CanergyAbilityController) CanergyAbilityController->ActivateSelectedAbility();
}

void AShooterCharacter::ActivateWeaponSpecial()
{
	if (!IsDead()) CanergyWeaponSpecial->ActivateSpecial();
}

void AShooterCharacter::ActivatePhaseAbility()
{
	if (!IsDead() && CanergyPhaseAbility) CanergyPhaseAbility->ActivatePhase();
}

void AShooterCharacter::RequestNextRound()
{
	if (IsPlayerControlled())
		if (auto* Mode = GetWorld()->GetAuthGameMode<AShooterGameMode>()) Mode->RequestNewRound();
}

void AShooterCharacter::InteractCarryCore()
{
	if (HasAuthority()) ServerInteractCarryCore_Implementation();
	else ServerInteractCarryCore();
}

void AShooterCharacter::ServerInteractCarryCore_Implementation()
{
	if (auto* Carry=Cast<ACanergyCarryPrototype>(UGameplayStatics::GetActorOfClass(GetWorld(),ACanergyCarryPrototype::StaticClass())))
		Carry->Interact(this);
}

void AShooterCharacter::StartSprint()
{
	bSprintHeld = true;
	UpdateMovementTuning();
}

void AShooterCharacter::StopSprint()
{
	bSprintHeld = false;
	UpdateMovementTuning();
}

void AShooterCharacter::StartLiquidDive() { SetLiquidDiveHeld(true); }
void AShooterCharacter::StopLiquidDive() { SetLiquidDiveHeld(false); }

void AShooterCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	if (bOffsetFirstPersonForPrototypeCrouch)
	{
		// The template camera is on the head socket; capsule crouch alone keeps that head standing.
		const FVector LocalOffset = GetMesh()->GetComponentTransform().InverseTransformVector(
			-FVector::UpVector * ScaledHalfHeightAdjust);
		GetFirstPersonMesh()->SetRelativeLocation(StandingFirstPersonRelativeLocation + LocalOffset);
	}
}

void AShooterCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	if (bOffsetFirstPersonForPrototypeCrouch)
		GetFirstPersonMesh()->SetRelativeLocation(StandingFirstPersonRelativeLocation);
}

void AShooterCharacter::SetLiquidDiveHeld(bool bHeld)
{
	bLiquidDiveHeld = bHeld && !IsDead() && !CanergyBubbleRespawn->IsBubbled();
	CanergyWallTraversal->SetDiveHeld(bLiquidDiveHeld);
	UpdateLiquidDive();
	UpdateMovementTuning();
}

void AShooterCharacter::UpdateLiquidDive()
{
	const bool bShouldDive = CainengGameRules::CanLiquidDive(bLiquidDiveHeld,
		GetCharacterMovement()->IsMovingOnGround(), CurrentTraversalMode == 0,
		IsDead() || CanergyBubbleRespawn->IsBubbled());
	if (bShouldDive == bLiquidDiving) return;
	bLiquidDiving = bShouldDive;
	if (bLiquidDiving)
	{
		Crouch();
		if (IsLocallyControlled()) ShowPrototypeFeedback(FText::FromString(TEXT("液态潜行 · 按住 Ctrl 高速穿行，松开恢复")), 1.8f);
	}
	else
	{
		// Let CharacterMovement keep a safe crouch if a ceiling blocks standing up.
		UnCrouch();
	}
}

bool AShooterCharacter::IsWallTraversing() const
{
	return CanergyWallTraversal && CanergyWallTraversal->IsWallTraversing();
}

void AShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear the respawn timer
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// base class handles move, aim and jump inputs
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Switch weapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Triggered, this, &AShooterCharacter::DoSwitchWeapon);
	}

	// Keep the core toy-weapon command independent from the template Blueprint's FireAction asset.
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AShooterCharacter::DoStartFiring);
	PlayerInputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &AShooterCharacter::DoStopFiring);
	PlayerInputComponent->BindKey(EKeys::R, IE_Pressed, this, &AShooterCharacter::CyclePaintMode);
	PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AShooterCharacter::DoSwitchWeapon);
	PlayerInputComponent->BindKey(EKeys::Z, IE_Pressed, this, &AShooterCharacter::CycleSelectedAbility);
	PlayerInputComponent->BindKey(EKeys::F, IE_Pressed, this, &AShooterCharacter::ActivateSelectedAbility);
	PlayerInputComponent->BindKey(EKeys::E, IE_Pressed, this, &AShooterCharacter::ActivateWeaponSpecial);
	PlayerInputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AShooterCharacter::RequestNextRound);
	PlayerInputComponent->BindKey(EKeys::G, IE_Pressed, this, &AShooterCharacter::InteractCarryCore);
	PlayerInputComponent->BindKey(EKeys::X, IE_Pressed, this, &AShooterCharacter::ActivatePhaseAbility);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &AShooterCharacter::StartSprint);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &AShooterCharacter::StopSprint);
	PlayerInputComponent->BindKey(EKeys::LeftControl, IE_Pressed, this, &AShooterCharacter::StartLiquidDive);
	PlayerInputComponent->BindKey(EKeys::LeftControl, IE_Released, this, &AShooterCharacter::StopLiquidDive);

}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority() || Damage <= 0.0f || (CanergyBubbleRespawn && CanergyBubbleRespawn->IsBubbled())) return 0.0f;
	// ignore if already dead
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	// Any successful hit reveals a phased ambusher before damage is resolved.
	if (CanergyPhaseAbility) CanergyPhaseAbility->BreakPhase();
	// Reduce HP
	if (auto* Mode = GetWorld()->GetAuthGameMode<AShooterGameMode>())
		if (auto* Carry = Mode->GetCarryPrototype()) Carry->Dislodge(this);
	CurrentHP -= Damage;

	// Have we depleted HP?
	if (CurrentHP <= 0.0f)
	{
		// Legacy template hazards also use the non-lethal return path.
		if (CanergyBubbleRespawn) CanergyBubbleRespawn->BeginBubbleAuthorityOnly();
	}

	// update the HUD
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));

	return Damage;
}

void AShooterCharacter::DoAim(float Yaw, float Pitch)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoAim(Yaw, Pitch);
	}
}

void AShooterCharacter::DoMove(float Right, float Forward)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		if (CanergyWallTraversal->HandleMove(Right, Forward)) return;
		Super::DoMove(Right, Forward);
	}
}

void AShooterCharacter::DoJumpStart()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		if (CanergyWallTraversal->TryWallJump()) return;
		Super::DoJumpStart();
	}
}

void AShooterCharacter::DoJumpEnd()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpEnd();
	}
}

void AShooterCharacter::DoStartFiring()
{
	if (IsDead()) return;
	if (CanergyPhaseAbility && !CanergyPhaseAbility->CanAttack())
	{
		ShowPrototypeFeedback(FText::FromString(TEXT("潜相出墙稳定中 · 暂不能开火")), 0.5f);
		return;
	}
	if (CanergyPhaseAbility) CanergyPhaseAbility->BreakPhase();
	if (CanergyToyWeaponController) CanergyToyWeaponController->StartFiring();
}

void AShooterCharacter::DoStopFiring()
{
	if (CanergyToyWeaponController) CanergyToyWeaponController->StopFiring();
}

void AShooterCharacter::DoSwitchWeapon()
{
	if (IsDead()) return;
	if (CanergyToyWeaponController)
	{
		const FText WeaponName = CanergyToyWeaponController->CycleWeapon();
		if (!WeaponName.IsEmpty())
		{
			ShowPrototypeFeedback(FText::Format(FText::FromString(TEXT("切换玩具武器：{0}")), WeaponName), 1.5f);
		}
		return;
	}

	// ensure we have at least two weapons two switch between
	if (OwnedWeapons.Num() > 1 && !IsDead())
	{
		// deactivate the old weapon
		CurrentWeapon->DeactivateWeapon();

		// find the index of the current weapon in the owned list
		int32 WeaponIndex = OwnedWeapons.Find(CurrentWeapon);

		// is this the last weapon?
		if (WeaponIndex == OwnedWeapons.Num() - 1)
		{
			// loop back to the beginning of the array
			WeaponIndex = 0;
		}
		else {
			// select the next weapon index
			++WeaponIndex;
		}

		// set the new weapon as current
		CurrentWeapon = OwnedWeapons[WeaponIndex];

		// activate the new weapon
		CurrentWeapon->ActivateWeapon(PlayerTag);
	}
}

void AShooterCharacter::AttachWeaponMeshes(AShooterWeapon* Weapon)
{
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// attach the weapon actor
	Weapon->AttachToActor(this, AttachmentRule);

	// attach the weapon meshes
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, FirstPersonWeaponSocket);
	
}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	// stub
}

void AShooterCharacter::AddWeaponRecoil(float Recoil)
{
	// apply the recoil as pitch input
	AddControllerPitchInput(Recoil);
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
	// trace ahead from the camera viewpoint
	FHitResult OutHit;

	const FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector End = Start + (GetFirstPersonCameraComponent()->GetForwardVector() * MaxAimDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	// return either the impact point or the trace end
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	// do we already own this weapon?
	AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass);

	if (!OwnedWeapon)
	{
		// spawn the new weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

		if (AddedWeapon)
		{
			// add the weapon to the owned list
			OwnedWeapons.Add(AddedWeapon);

			// if we have an existing weapon, deactivate it
			if (CurrentWeapon)
			{
				CurrentWeapon->DeactivateWeapon();
			}

			// switch to the new weapon
			CurrentWeapon = AddedWeapon;
			CurrentWeapon->ActivateWeapon(PlayerTag);
		}
	}
}

void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	// update the bullet counter
	OnBulletCountUpdated.Broadcast(Weapon->GetMagazineSize(), Weapon->GetBulletCount());

	// set the character mesh AnimInstances
	GetFirstPersonMesh()->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
	// unused
}

void AShooterCharacter::OnSemiWeaponRefire()
{
	// unused
}

AShooterWeapon* AShooterCharacter::FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	// check each owned weapon
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	// weapon not found
	return nullptr;

}

void AShooterCharacter::Die()
{
	// deactivate the weapon
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
	}

	// increment the team score
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->IncrementTeamScore(TeamByte);
	}

	// grant the death tag to the character
	Tags.Add(DeathTag);
		
	// stop character movement
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	// disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// disable controls
	DisableInput(nullptr);

	// reset the bullet counter UI
	OnBulletCountUpdated.Broadcast(0, 0);

	// call the BP handler
	BP_OnDeath();

	// schedule character respawn
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterCharacter::OnRespawn, RespawnTime, false);
}

void AShooterCharacter::OnRespawn()
{
	// destroy the character to force the PC to respawn
	Destroy();
}

bool AShooterCharacter::IsDead() const
{
	// the character is dead if their current HP drops to zero
	return CurrentHP <= 0.0f;
}

void AShooterCharacter::SetTeam(uint8 Team)
{
	TeamByte = Team;
}
