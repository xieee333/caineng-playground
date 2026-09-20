// Copyright Epic Games, Inc. All Rights Reserved.

#include "Variant_Shooter/Gameplay/CanergyRuntimeComponents.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "TP_FirstPersonCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Variant_Shooter/ShooterGameMode.h"
#include "Variant_Shooter/ShooterCharacter.h"
#include "Variant_Shooter/Gameplay/CarryPrototype.h"
#include "Variant_Shooter/Gameplay/GameplayDefinitionAssets.h"
#include "Variant_Shooter/Gameplay/InteractionEffectRules.h"
#include "Variant_Shooter/Gameplay/PaintSurfaceRules.h"
#include "WallTraversalComponent.h"

namespace
{
	void ApplyMaterialOverrides(USkeletalMeshComponent* MeshComponent,
		const TArray<TSoftObjectPtr<UMaterialInterface>>& MaterialOverrides)
	{
		if (!MeshComponent) return;
		for (int32 Index = 0; Index < MaterialOverrides.Num(); ++Index)
		{
			if (UMaterialInterface* Material = MaterialOverrides[Index].LoadSynchronous())
			{
				MeshComponent->SetMaterial(Index, Material);
			}
		}
	}
}

UCanergyCharacterProfileComponent::UCanergyCharacterProfileComponent()
{
	SetIsReplicatedByDefault(true);
}

void UCanergyCharacterProfileComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyDefinitions();
}

bool UCanergyCharacterProfileComponent::ApplyDefinitions()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return false;
	}

	LoadedGameplayDefinition = GameplayDefinition.LoadSynchronous();
	if (LoadedGameplayDefinition)
	{
		const FCanergyMovementTuning& Movement = LoadedGameplayDefinition->Movement;
		Character->GetCharacterMovement()->MaxWalkSpeed = FMath::Max(100.0f, Movement.GroundSpeed);
		Character->GetCharacterMovement()->AirControl = FMath::Clamp(Movement.AirControl, 0.0f, 1.0f);
		Character->GetCharacterMovement()->JumpZVelocity = FMath::Max(0.0f, Movement.JumpVelocity);
		Character->GetCapsuleComponent()->SetCapsuleSize(
			FMath::Max(10.0f, Movement.CapsuleRadius),
			FMath::Max(20.0f, Movement.CapsuleHalfHeight));
	}

	LoadedVisualProfile = VisualProfileOverride.LoadSynchronous();
	if (!LoadedVisualProfile && LoadedGameplayDefinition)
	{
		LoadedVisualProfile = LoadedGameplayDefinition->DefaultVisualProfile.LoadSynchronous();
	}
	if (!LoadedVisualProfile)
	{
		return LoadedGameplayDefinition != nullptr;
	}

	USkeletalMeshComponent* ThirdPersonMesh = Character->GetMesh();
	if (USkeletalMesh* BodyMesh = LoadedVisualProfile->BodyMesh.LoadSynchronous())
	{
		ThirdPersonMesh->SetSkeletalMeshAsset(BodyMesh);
		ThirdPersonMesh->SetRelativeTransform(LoadedVisualProfile->BodyMeshRelativeTransform);
		ThirdPersonMesh->SetAnimInstanceClass(LoadedVisualProfile->ThirdPersonAnimationClass.LoadSynchronous());
	}
	ApplyMaterialOverrides(ThirdPersonMesh, LoadedVisualProfile->MaterialOverrides);

	if (ATP_FirstPersonCharacter* FirstPersonCharacter = Cast<ATP_FirstPersonCharacter>(Character))
	{
		USkeletalMeshComponent* FirstPersonMesh = FirstPersonCharacter->GetFirstPersonMesh();
		if (USkeletalMesh* ArmsMesh = LoadedVisualProfile->FirstPersonMesh.LoadSynchronous())
		{
			FirstPersonMesh->SetSkeletalMeshAsset(ArmsMesh);
			FirstPersonMesh->SetRelativeTransform(LoadedVisualProfile->FirstPersonMeshRelativeTransform);
			FirstPersonMesh->SetAnimInstanceClass(LoadedVisualProfile->FirstPersonAnimationClass.LoadSynchronous());
		}
		ApplyMaterialOverrides(FirstPersonMesh, LoadedVisualProfile->FirstPersonMaterialOverrides);
	}
	else if (!LoadedVisualProfile->FirstPersonMesh.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("Character visual profile '%s' defines first-person mesh, but owner '%s' has no first-person mesh component."),
			*GetNameSafe(LoadedVisualProfile), *GetNameSafe(Character));
	}
	return true;
}

UCanergyPaintableSurfaceComponent::UCanergyPaintableSurfaceComponent()
{
	SetIsReplicatedByDefault(true);
}

void UCanergyPaintableSurfaceComponent::BeginPlay()
{
	Super::BeginPlay();
	if (SurfaceKind != ECanergySurfaceKind::None)
	{
		UpdateSurfacePresentation();
	}
}

bool UCanergyPaintableSurfaceComponent::ApplySurfaceKindAuthorityOnly(ECanergySurfaceKind NewKind)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || NewKind == ECanergySurfaceKind::None || SurfaceKind == NewKind)
	{
		return false;
	}

	SurfaceKind = NewKind;
	UpdateSurfacePresentation();
	OnSurfaceKindChanged.Broadcast(SurfaceKind);
	Owner->ForceNetUpdate();
	return true;
}

bool UCanergyPaintableSurfaceComponent::ClearSurfaceAuthorityOnly()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || SurfaceKind == ECanergySurfaceKind::None)
	{
		return false;
	}

	SurfaceKind = ECanergySurfaceKind::None;
	UpdateSurfacePresentation();
	OnSurfaceKindChanged.Broadcast(SurfaceKind);
	Owner->ForceNetUpdate();
	return true;
}

void UCanergyPaintableSurfaceComponent::OnRep_SurfaceKind()
{
	UpdateSurfacePresentation();
	OnSurfaceKindChanged.Broadcast(SurfaceKind);
}

void UCanergyPaintableSurfaceComponent::UpdateSurfacePresentation()
{
	FLinearColor SurfaceColor = FLinearColor::White;
	switch (SurfaceKind)
	{
	case ECanergySurfaceKind::None: SurfaceColor = FLinearColor::White; break;
	case ECanergySurfaceKind::Liquid: SurfaceColor = FLinearColor(0.02f, 0.78f, 1.0f); break;
	case ECanergySurfaceKind::Bounce: SurfaceColor = FLinearColor(1.0f, 0.43f, 0.04f); break;
	case ECanergySurfaceKind::Float: SurfaceColor = FLinearColor(1.0f, 0.08f, 0.56f); break;
	case ECanergySurfaceKind::Sticky: SurfaceColor = FLinearColor(0.35f, 0.95f, 0.24f); break;
	case ECanergySurfaceKind::Mirror: SurfaceColor = FLinearColor(0.72f, 0.85f, 1.0f); break;
	case ECanergySurfaceKind::Conductive: SurfaceColor = FLinearColor(0.94f, 0.9f, 0.18f); break;
	default: return;
	}

	AActor* Owner = GetOwner();
	if (!Owner) return;
	TArray<UMeshComponent*> MeshComponents;
	Owner->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!MeshComponent) continue;
		for (int32 MaterialIndex = 0; MaterialIndex < MeshComponent->GetNumMaterials(); ++MaterialIndex)
		{
			if (UMaterialInstanceDynamic* DynamicMaterial = MeshComponent->CreateAndSetMaterialInstanceDynamic(MaterialIndex))
			{
				DynamicMaterial->SetVectorParameterValue(FName("Color"), SurfaceColor);
				DynamicMaterial->SetVectorParameterValue(FName("BaseColor"), SurfaceColor);
				DynamicMaterial->SetVectorParameterValue(FName("EmissiveColor"), SurfaceColor * 0.35f);
			}
		}
	}
}

void UCanergyPaintableSurfaceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCanergyPaintableSurfaceComponent, SurfaceKind);
}

UCanergyInteractionComponent::UCanergyInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
	SetIsReplicatedByDefault(true);
	SetComponentTickEnabled(false);
}

bool UCanergyInteractionComponent::ApplyEffectAuthorityOnly(const FCanergyInteractionEffectSpec& EffectSpec)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority()
		|| EffectSpec.Kind == ECanergyInteractionKind::None
		|| EffectSpec.DurationSeconds <= 0.0f)
	{
		return false;
	}

	ActiveEffects = CainengGameRules::ApplyInteractionEffect(ActiveEffects, EffectSpec);
	SetComponentTickEnabled(!ActiveEffects.IsEmpty());
	OnEffectsChanged.Broadcast();
	Owner->ForceNetUpdate();
	return true;
}

bool UCanergyInteractionComponent::ClearEffectAuthorityOnly(ECanergyInteractionKind Kind)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || Kind == ECanergyInteractionKind::None)
	{
		return false;
	}

	const int32 RemovedCount = ActiveEffects.RemoveAll([Kind](const FCanergyActiveInteractionEffect& Effect)
	{
		return Effect.Kind == Kind;
	});
	if (RemovedCount <= 0)
	{
		return false;
	}

	SetComponentTickEnabled(!ActiveEffects.IsEmpty());
	OnEffectsChanged.Broadcast();
	Owner->ForceNetUpdate();
	return true;
}

void UCanergyInteractionComponent::ClearAllEffectsAuthorityOnly()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || ActiveEffects.IsEmpty())
	{
		return;
	}

	ActiveEffects.Reset();
	SetComponentTickEnabled(false);
	OnEffectsChanged.Broadcast();
	Owner->ForceNetUpdate();
}

bool UCanergyInteractionComponent::HasEffect(ECanergyInteractionKind Kind) const
{
	return ActiveEffects.ContainsByPredicate([Kind](const FCanergyActiveInteractionEffect& Effect)
	{
		return Effect.Kind == Kind && Effect.IsActive();
	});
}

float UCanergyInteractionComponent::GetEffectRemainingSeconds(ECanergyInteractionKind Kind) const
{
	for (const FCanergyActiveInteractionEffect& Effect : ActiveEffects)
	{
		if (Effect.Kind == Kind && Effect.IsActive())
		{
			return Effect.RemainingSeconds;
		}
	}
	return 0.0f;
}

void UCanergyInteractionComponent::OnRep_ActiveEffects()
{
	SetComponentTickEnabled(!ActiveEffects.IsEmpty() && GetOwner() && GetOwner()->HasAuthority());
	OnEffectsChanged.Broadcast();
}

void UCanergyInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	const int32 PreviousCount = ActiveEffects.Num();
	ActiveEffects = CainengGameRules::TickInteractionEffects(ActiveEffects, DeltaTime);
	if (ActiveEffects.Num() != PreviousCount)
	{
		OnEffectsChanged.Broadcast();
		Owner->ForceNetUpdate();
	}
	if (ActiveEffects.IsEmpty())
	{
		SetComponentTickEnabled(false);
	}
}

void UCanergyInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCanergyInteractionComponent, ActiveEffects);
}

UCanergyJoyScoreComponent::UCanergyJoyScoreComponent()
{
	SetIsReplicatedByDefault(true);
}

int32 UCanergyJoyScoreComponent::AwardJoyAuthorityOnly(ECanergyJoyAction Action)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return 0;
	}
	if (const auto* Mode = Owner->GetWorld()->GetAuthGameMode<AShooterGameMode>();
		Mode && Mode->GetMatchRuntimeState().Phase == ECanergyMatchPhase::Finished) return 0;

	const int32 Points = CainengGameRules::GetJoyAward(Action);
	if (Points <= 0)
	{
		return 0;
	}

	JoyScore += Points;
	JoyContributions.FindOrAdd(Action) += Points;
	OnJoyScoreChanged.Broadcast(JoyScore);
	Owner->ForceNetUpdate();
	if (AShooterGameMode* GameMode = Cast<AShooterGameMode>(Owner->GetWorld()->GetAuthGameMode()))
	{
		GameMode->AddCarnivalMeter(static_cast<float>(Points));
	}
	return Points;
}

void UCanergyJoyScoreComponent::ResetJoyScoreAuthorityOnly()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || JoyScore == 0)
	{
		return;
	}

	JoyScore = 0;
	JoyContributions.Reset();
	OnJoyScoreChanged.Broadcast(JoyScore);
	Owner->ForceNetUpdate();
}

void UCanergyJoyScoreComponent::OnRep_JoyScore()
{
	OnJoyScoreChanged.Broadcast(JoyScore);
}

void UCanergyJoyScoreComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCanergyJoyScoreComponent, JoyScore);
}

UCanergyBubbleRespawnComponent::UCanergyBubbleRespawnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
	SetIsReplicatedByDefault(true);
	SetComponentTickEnabled(false);
}

bool UCanergyBubbleRespawnComponent::BeginBubbleAuthorityOnly()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || RespawnState.bIsBubbled)
	{
		return false;
	}

	RespawnState.bIsBubbled = true;
	// End surface flight before snapshotting the respawn movement mode.
	if (auto* Wall = Owner->FindComponentByClass<UCanergyWallTraversalComponent>()) Wall->SetDiveHeld(false);
	if (auto* Weapon = Owner->FindComponentByClass<UCanergyToyWeaponControllerComponent>()) Weapon->StopFiring();
	if (auto* Interaction = Owner->FindComponentByClass<UCanergyInteractionComponent>()) Interaction->ClearAllEffectsAuthorityOnly();
	RespawnState.TimeInBubbleSeconds = 0.0f;
	RespawnState.bRespawnReady = false;
	ReadyDisplaySeconds = 0.0f;
	SafeRespawnTransform = Owner->GetActorTransform();
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			SavedMovementMode = static_cast<uint8>(Movement->MovementMode);
			SavedCustomMovementMode = Movement->CustomMovementMode;
			Movement->StopMovementImmediately();
			Movement->DisableMovement();
		}
		if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			SavedCapsuleCollision = Capsule->GetCollisionEnabled();
			Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	bSavedActorHidden = Owner->IsHidden();

	TArray<AActor*> PlayerZeroStarts;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), FName("Player0"), PlayerZeroStarts);
	if (!PlayerZeroStarts.IsEmpty())
	{
		const AActor* Start = PlayerZeroStarts[0];
		SafeRespawnTransform = Start->GetActorTransform();
		SafeRespawnTransform.AddToTranslation(Start->GetActorRightVector() * 180.0f + Start->GetActorForwardVector() * 100.0f);
	}
	else
	{
		TArray<AActor*> Starts;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), Starts);
		if (!Starts.IsEmpty())
		{
			Starts.Sort([Owner](const AActor& A, const AActor& B)
			{
				return FVector::DistSquared(A.GetActorLocation(), Owner->GetActorLocation())
					< FVector::DistSquared(B.GetActorLocation(), Owner->GetActorLocation());
			});
			SafeRespawnTransform = Starts[0]->GetActorTransform();
			SafeRespawnTransform.AddToTranslation(Starts[0]->GetActorRightVector() * 180.0f);
		}
	}
	SetComponentTickEnabled(true);
	OnBubbleStateChanged.Broadcast();
	Owner->ForceNetUpdate();
	return true;
}

bool UCanergyBubbleRespawnComponent::CompleteRespawnAuthorityOnly()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !RespawnState.bRespawnReady)
	{
		return false;
	}

	Owner->SetActorHiddenInGame(bSavedActorHidden);
	Owner->SetActorLocationAndRotation(SafeRespawnTransform.GetLocation(), SafeRespawnTransform.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
	if (ACharacter* Character = Cast<ACharacter>(Owner))
	{
		if (UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
		{
			Capsule->SetCollisionEnabled(SavedCapsuleCollision);
		}
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			Movement->SetMovementMode(static_cast<EMovementMode>(SavedMovementMode), SavedCustomMovementMode);
		}
	}
	RespawnState = FCanergyBubbleRespawnSnapshot{};
	ReadyDisplaySeconds = 0.0f;
	SetComponentTickEnabled(false);
	OnBubbleStateChanged.Broadcast();
	Owner->ForceNetUpdate();
	return true;
}

float UCanergyBubbleRespawnComponent::GetRespawnSecondsRemaining() const
{
	if (!RespawnState.bIsBubbled || RespawnState.bRespawnReady)
	{
		return 0.0f;
	}
	return FMath::Max(0.0f, CainengGameRules::BubbleRespawnDelaySeconds - RespawnState.TimeInBubbleSeconds);
}

void UCanergyBubbleRespawnComponent::OnRep_RespawnState()
{
	OnBubbleStateChanged.Broadcast();
	if (RespawnState.bRespawnReady)
	{
		OnRespawnReady.Broadcast();
	}
}

void UCanergyBubbleRespawnComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !RespawnState.bIsBubbled)
	{
		SetComponentTickEnabled(false);
		return;
	}
	if (RespawnState.bRespawnReady)
	{
		ReadyDisplaySeconds += FMath::Max(0.0f, DeltaTime);
		if (ReadyDisplaySeconds >= 0.1f) CompleteRespawnAuthorityOnly();
		return;
	}

	RespawnState.TimeInBubbleSeconds = FMath::Min(CainengGameRules::BubbleRespawnDelaySeconds,
		RespawnState.TimeInBubbleSeconds + FMath::Max(0.0f, DeltaTime));
	if (CainengGameRules::CanRespawnFromBubble(RespawnState.TimeInBubbleSeconds))
	{
		RespawnState.bRespawnReady = true;
		OnRespawnReady.Broadcast();
	}
	Owner->ForceNetUpdate();
}

void UCanergyBubbleRespawnComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCanergyBubbleRespawnComponent, RespawnState);
}

UCanergyAbilityControllerComponent::UCanergyAbilityControllerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.15f;
	SetIsReplicatedByDefault(true);
	SetComponentTickEnabled(false);
}

void UCanergyAbilityControllerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AbilityLoadout.IsEmpty())
	{
		if (UCanergyCharacterProfileComponent* Profile = GetOwner()
			? GetOwner()->FindComponentByClass<UCanergyCharacterProfileComponent>() : nullptr)
		{
			if (!Profile->GetGameplayDefinition()) Profile->ApplyDefinitions();
			if (UCanergyCharacterGameplayDefinition* GameplayDefinition = Profile->GetGameplayDefinition())
			{
				for (const TSoftObjectPtr<UCanergyAbilityDefinition>& SoftAbility : GameplayDefinition->Abilities)
				{
					if (UCanergyAbilityDefinition* Ability = SoftAbility.LoadSynchronous())
					{
						AbilityLoadout.Add(Ability);
					}
				}
			}
		}
	}

	if (AbilityLoadout.IsEmpty())
	{
		BuildRuntimePrototypeDefinitions();
	}

	AbilityReadyAtServerTimes.Init(0.0f, AbilityLoadout.IsEmpty()
		? RuntimePrototypeDefinitions.Num() : AbilityLoadout.Num());
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ActiveAbilityIndex = FMath::Clamp(ActiveAbilityIndex, 0,
			FMath::Max(0, AbilityReadyAtServerTimes.Num() - 1));
		UpdateSelectedCooldownReplication();
	}
	else
	{
		OnRep_AbilitySelection();
	}
}

void UCanergyAbilityControllerComponent::BuildRuntimePrototypeDefinitions()
{
	static const TCHAR* AbilityNames[] =
	{
		TEXT("炮弹化"), TEXT("墙内藏身"), TEXT("位置交换"),
		TEXT("彩能反转"), TEXT("召唤宠物"), TEXT("超级喷射")
	};
	static const ECanergyAbilityAction Actions[] =
	{
		ECanergyAbilityAction::Cannonball, ECanergyAbilityAction::WallHide,
		ECanergyAbilityAction::SwapPosition, ECanergyAbilityAction::InvertSurface,
		ECanergyAbilityAction::SummonPet, ECanergyAbilityAction::SuperSpray
	};
	static const float Cooldowns[] = { 8.0f, 10.0f, 9.0f, 8.0f, 16.0f, 12.0f };
	static const float Durations[] = { 1.2f, 1.0f, 0.0f, 0.0f, 12.0f, 3.5f };
	static const float Radii[] = { 0.0f, 1500.0f, 1800.0f, 2400.0f, 1800.0f, 0.0f };
	static const float Magnitudes[] = { 1150.0f, 0.0f, 0.0f, 0.0f, 420.0f, 0.42f };

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Actions); ++Index)
	{
		UCanergyAbilityDefinition* Definition = NewObject<UCanergyAbilityDefinition>(this,
			FName(*FString::Printf(TEXT("PrototypeAbility_%d"), Index)));
		Definition->Action = Actions[Index];
		Definition->DisplayName = FText::FromString(AbilityNames[Index]);
		Definition->CooldownSeconds = Cooldowns[Index];
		Definition->DurationSeconds = Durations[Index];
		Definition->Radius = Radii[Index];
		Definition->Magnitude = Magnitudes[Index];
		RuntimePrototypeDefinitions.Add(Definition);
	}
}

float UCanergyAbilityControllerComponent::GetServerWorldTime() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const AGameStateBase* GameState = World->GetGameState())
		{
			return GameState->GetServerWorldTimeSeconds();
		}
		return World->GetTimeSeconds();
	}
	return 0.0f;
}

ACharacter* UCanergyAbilityControllerComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

FText UCanergyAbilityControllerComponent::GetSelectedAbilityName() const
{
	const TArray<TObjectPtr<UCanergyAbilityDefinition>>& Definitions = AbilityLoadout.IsEmpty()
		? RuntimePrototypeDefinitions : AbilityLoadout;
	return Definitions.IsValidIndex(ActiveAbilityIndex) && Definitions[ActiveAbilityIndex]
		? Definitions[ActiveAbilityIndex]->DisplayName : FText::GetEmpty();
}

float UCanergyAbilityControllerComponent::GetSelectedAbilityCooldownRemaining() const
{
	return FMath::Max(0.0f, SelectedAbilityReadyAtServerTime - GetServerWorldTime());
}

FText UCanergyAbilityControllerComponent::CycleAbility()
{
	const TArray<TObjectPtr<UCanergyAbilityDefinition>>& Definitions = AbilityLoadout.IsEmpty()
		? RuntimePrototypeDefinitions : AbilityLoadout;
	if (Definitions.IsEmpty()) return FText::GetEmpty();
	const int32 NextIndex = (ActiveAbilityIndex + 1) % Definitions.Num();
	if (GetOwner() && GetOwner()->HasAuthority()) CycleAbilityAuthorityOnly();
	else ServerCycleAbility();
	return Definitions.IsValidIndex(NextIndex) && Definitions[NextIndex]
		? Definitions[NextIndex]->DisplayName : FText::GetEmpty();
}

void UCanergyAbilityControllerComponent::ServerCycleAbility_Implementation()
{
	CycleAbilityAuthorityOnly();
}

void UCanergyAbilityControllerComponent::CycleAbilityAuthorityOnly()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	const int32 DefinitionCount = AbilityLoadout.IsEmpty()
		? RuntimePrototypeDefinitions.Num() : AbilityLoadout.Num();
	if (DefinitionCount <= 0) return;
	ActiveAbilityIndex = (ActiveAbilityIndex + 1) % DefinitionCount;
	UpdateSelectedCooldownReplication();
	const float Remaining = GetSelectedAbilityCooldownRemaining();
	const FText Status = Remaining > 0.0f
		? FText::Format(FText::FromString(TEXT("技能选择 · {0} · 冷却 {1} 秒")),
			GetSelectedAbilityName(), FText::AsNumber(FMath::CeilToInt(Remaining)))
		: FText::Format(FText::FromString(TEXT("技能选择 · {0} · 按 F 发动")), GetSelectedAbilityName());
	ShowAbilityFeedback(Status);
}

bool UCanergyAbilityControllerComponent::ActivateSelectedAbility()
{
	AActor* Owner = GetOwner();
	if (!Owner) return false;
	if (Owner->HasAuthority()) return ActivateSelectedAbilityAuthorityOnly();
	ServerActivateSelectedAbility();
	return true;
}

void UCanergyAbilityControllerComponent::ServerActivateSelectedAbility_Implementation()
{
	ActivateSelectedAbilityAuthorityOnly();
}

bool UCanergyAbilityControllerComponent::ActivateSelectedAbilityAuthorityOnly()
{
	AActor* Owner = GetOwner();
	if (Owner)
	{
		const auto* Bubble = Owner->FindComponentByClass<UCanergyBubbleRespawnComponent>();
		if (Bubble && Bubble->IsBubbled())
		{
			ShowAbilityFeedback(FText::FromString(TEXT("泡泡回场中 · 回场后可发动技能")));
			return false;
		}
	}
	const TArray<TObjectPtr<UCanergyAbilityDefinition>>& Definitions = AbilityLoadout.IsEmpty()
		? RuntimePrototypeDefinitions : AbilityLoadout;
	if (!Owner || !Owner->HasAuthority() || !Definitions.IsValidIndex(ActiveAbilityIndex)
		|| !Definitions[ActiveAbilityIndex]) return false;

	const float Now = GetServerWorldTime();
	const float ReadyAt = AbilityReadyAtServerTimes.IsValidIndex(ActiveAbilityIndex)
		? AbilityReadyAtServerTimes[ActiveAbilityIndex] : 0.0f;
	if (Now + KINDA_SMALL_NUMBER < ReadyAt)
	{
		ShowAbilityFeedback(FText::Format(FText::FromString(TEXT("技能还在充能 · {0} 秒")),
			FText::AsNumber(FMath::CeilToInt(ReadyAt - Now))));
		return false;
	}

	const UCanergyAbilityDefinition* Definition = Definitions[ActiveAbilityIndex];
	if (!ExecuteAbilityAuthorityOnly(Definition)) return false;

	if (AbilityReadyAtServerTimes.IsValidIndex(ActiveAbilityIndex))
	{
		AbilityReadyAtServerTimes[ActiveAbilityIndex] = Now + FMath::Max(0.0f, Definition->CooldownSeconds);
	}
	UpdateSelectedCooldownReplication();
	ShowAbilityFeedback(FText::Format(FText::FromString(TEXT("技能发动 · {0}")), Definition->DisplayName));
	return true;
}

bool UCanergyAbilityControllerComponent::ExecuteAbilityAuthorityOnly(const UCanergyAbilityDefinition* Definition)
{
	ACharacter* Character = GetOwnerCharacter();
	UWorld* World = GetWorld();
	if (!Character || !World || !Definition || !Character->HasAuthority()) return false;

	FVector TraceStart = Character->GetPawnViewLocation();
	FRotator ViewRotation = Character->GetActorRotation();
	if (AController* Controller = Character->GetController())
	{
		Controller->GetPlayerViewPoint(TraceStart, ViewRotation);
	}
	const FVector AimDirection = ViewRotation.Vector().GetSafeNormal();
	const float TraceDistance = FMath::Max(300.0f, Definition->Radius > 0.0f ? Definition->Radius : 1600.0f);
	FHitResult AimHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CanergyAbilityAim), false, Character);
	const bool bHasAimHit = World->LineTraceSingleByChannel(AimHit, TraceStart,
		TraceStart + AimDirection * TraceDistance, ECC_Visibility, QueryParams);

	switch (Definition->Action)
	{
	case ECanergyAbilityAction::Cannonball:
	{
		FVector HorizontalDirection = AimDirection.GetSafeNormal2D();
		if (HorizontalDirection.IsNearlyZero()) HorizontalDirection = Character->GetActorForwardVector();
		Character->LaunchCharacter(HorizontalDirection * FMath::Max(500.0f, Definition->Magnitude)
			+ FVector::UpVector * 760.0f, true, true);
		return true;
	}
	case ECanergyAbilityAction::WallHide:
	{
		AActor* SurfaceActor = bHasAimHit ? AimHit.GetActor() : nullptr;
		UCanergyPaintableSurfaceComponent* Surface = SurfaceActor
			? SurfaceActor->FindComponentByClass<UCanergyPaintableSurfaceComponent>() : nullptr;
		if (!Surface || !Surface->IsPainted())
		{
			ShowAbilityFeedback(FText::FromString(TEXT("墙内藏身需要瞄准已喷绘的墙面")));
			return false;
		}
		const float CapsuleRadius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
		const FVector ExitLocation = AimHit.ImpactPoint - AimHit.ImpactNormal * (CapsuleRadius + 90.0f);
		if (!Character->TeleportTo(ExitLocation, Character->GetActorRotation(), false, false))
		{
			ShowAbilityFeedback(FText::FromString(TEXT("墙后空间不足 · 换一面喷绘墙再试")));
			return false;
		}
		return true;
	}
	case ECanergyAbilityAction::SwapPosition:
	{
		TArray<AActor*> Characters;
		UGameplayStatics::GetAllActorsOfClass(World, ACharacter::StaticClass(), Characters);
		ACharacter* OtherCharacter = nullptr;
		float NearestDistanceSquared = FMath::Square(Definition->Radius > 0.0f ? Definition->Radius : 1800.0f);
		for (AActor* Actor : Characters)
		{
			ACharacter* Candidate = Cast<ACharacter>(Actor);
			if (!Candidate || Candidate == Character || Candidate->IsActorBeingDestroyed()) continue;
			if (const UCanergyBubbleRespawnComponent* Respawn = Candidate->FindComponentByClass<UCanergyBubbleRespawnComponent>();
				Respawn && Respawn->IsBubbled()) continue;
			const float DistanceSquared = FVector::DistSquared(Character->GetActorLocation(), Candidate->GetActorLocation());
			const FVector DirectionToCandidate = (Candidate->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal();
			if (DistanceSquared < NearestDistanceSquared && FVector::DotProduct(AimDirection, DirectionToCandidate) > 0.35f)
			{
				NearestDistanceSquared = DistanceSquared;
				OtherCharacter = Candidate;
			}
		}
		if (OtherCharacter)
		{
			const FTransform PlayerTransform = Character->GetActorTransform();
			const FTransform OtherTransform = OtherCharacter->GetActorTransform();
			const bool bPlayerCollisionEnabled = Character->GetActorEnableCollision();
			const bool bOtherCollisionEnabled = OtherCharacter->GetActorEnableCollision();
			Character->SetActorEnableCollision(false);
			OtherCharacter->SetActorEnableCollision(false);
			Character->SetActorLocationAndRotation(OtherTransform.GetLocation(), OtherTransform.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
			OtherCharacter->SetActorLocationAndRotation(PlayerTransform.GetLocation(), PlayerTransform.Rotator(), false, nullptr, ETeleportType::TeleportPhysics);
			Character->SetActorEnableCollision(bPlayerCollisionEnabled);
			OtherCharacter->SetActorEnableCollision(bOtherCollisionEnabled);
			Character->GetCharacterMovement()->StopMovementImmediately();
			OtherCharacter->GetCharacterMovement()->StopMovementImmediately();
			return true;
		}

		if (!bHasAimHit)
		{
			ShowAbilityFeedback(FText::FromString(TEXT("没有可交换的目标 · 瞄准一处地面或玩家")));
			return false;
		}
		const float SurfaceOffset = FMath::Abs(AimHit.ImpactNormal.Z) > 0.7f
			? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 8.0f
			: Character->GetCapsuleComponent()->GetScaledCapsuleRadius() + 8.0f;
		if (!Character->TeleportTo(AimHit.ImpactPoint + AimHit.ImpactNormal * SurfaceOffset,
			Character->GetActorRotation(), false, false))
		{
			ShowAbilityFeedback(FText::FromString(TEXT("标记位置被挡住了 · 换个落点再试")));
			return false;
		}
		Character->GetCharacterMovement()->StopMovementImmediately();
		return true;
	}
	case ECanergyAbilityAction::InvertSurface:
	{
		UCanergyPaintableSurfaceComponent* Surface = bHasAimHit && AimHit.GetActor()
			? AimHit.GetActor()->FindComponentByClass<UCanergyPaintableSurfaceComponent>() : nullptr;
		if (!Surface || !Surface->IsPainted())
		{
			ShowAbilityFeedback(FText::FromString(TEXT("彩能反转需要瞄准已喷绘表面")));
			return false;
		}
		ECanergySurfaceKind InvertedKind = ECanergySurfaceKind::None;
		switch (Surface->GetSurfaceKind())
		{
		case ECanergySurfaceKind::Liquid: InvertedKind = ECanergySurfaceKind::Bounce; break;
		case ECanergySurfaceKind::Bounce: InvertedKind = ECanergySurfaceKind::Liquid; break;
		case ECanergySurfaceKind::Float: InvertedKind = ECanergySurfaceKind::Sticky; break;
		case ECanergySurfaceKind::Sticky: InvertedKind = ECanergySurfaceKind::Float; break;
		case ECanergySurfaceKind::Mirror: InvertedKind = ECanergySurfaceKind::Conductive; break;
		case ECanergySurfaceKind::Conductive: InvertedKind = ECanergySurfaceKind::Mirror; break;
		default: break;
		}
		if (InvertedKind == ECanergySurfaceKind::None || !Surface->ApplySurfaceKindAuthorityOnly(InvertedKind))
		{
			ShowAbilityFeedback(FText::FromString(TEXT("这类表面暂时无法反转")));
			return false;
		}
		if (UCanergyJoyScoreComponent* Joy = Character->FindComponentByClass<UCanergyJoyScoreComponent>())
		{
			Joy->AwardJoyAuthorityOnly(ECanergyJoyAction::EnvironmentalChain);
		}
		return true;
	}
	case ECanergyAbilityAction::SummonPet:
	{
		if (ActivePet.IsValid())
		{
			ShowAbilityFeedback(FText::FromString(TEXT("彩能伙伴还在场上帮忙")));
			return false;
		}
		UClass* PetClass = Definition->SpawnedActorClass.LoadSynchronous();
		FVector ViewLocation = Character->GetPawnViewLocation();
		FRotator PetSpawnViewRotation = Character->GetActorRotation();
		if (AController* Controller = Character->GetController())
		{
			Controller->GetPlayerViewPoint(ViewLocation, PetSpawnViewRotation);
		}
		const FVector SpawnLocation = ViewLocation + PetSpawnViewRotation.Vector() * 260.0f
			- PetSpawnViewRotation.RotateVector(FVector::RightVector) * 75.0f + FVector::UpVector * 25.0f;
		const FTransform SpawnTransform(Character->GetActorRotation(), SpawnLocation);
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = Character;
		SpawnParameters.Instigator = Character;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Pet = PetClass
			? World->SpawnActor<AActor>(PetClass, SpawnTransform, SpawnParameters)
			: World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), SpawnTransform, SpawnParameters);
		if (!Pet)
		{
			ShowAbilityFeedback(FText::FromString(TEXT("召唤失败 · 再试一次")));
			return false;
		}
		Pet->SetReplicates(true);
		Pet->SetReplicateMovement(true);
		Pet->SetActorHiddenInGame(false);
		Pet->Tags.AddUnique(FName("CanergyPet"));
		if (AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Pet))
		{
			UStaticMeshComponent* Mesh = MeshActor->GetStaticMeshComponent();
			Mesh->SetMobility(EComponentMobility::Movable);
			Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Mesh->SetSimulatePhysics(false);
			Mesh->SetCastShadow(false);
			Mesh->SetVisibility(true, true);
			Mesh->SetHiddenInGame(false, true);
			if (UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr,
				TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
			{
				Mesh->SetMaterial(0, BaseMaterial);
				if (UMaterialInstanceDynamic* DynamicMaterial = Mesh->CreateAndSetMaterialInstanceDynamic(0))
				{
					const FLinearColor PetColor(0.25f, 0.92f, 1.0f);
					DynamicMaterial->SetVectorParameterValue(FName("Color"), PetColor);
					DynamicMaterial->SetVectorParameterValue(FName("BaseColor"), PetColor);
					DynamicMaterial->SetVectorParameterValue(FName("EmissiveColor"), PetColor * 0.8f);
				}
			}
			Pet->SetActorScale3D(FVector(0.36f));
		}
		ActivePet = Pet;
		ActivePetEndTime = GetServerWorldTime() + FMath::Max(1.0f, Definition->DurationSeconds);
		ActivePetChaseRadius = FMath::Max(300.0f, Definition->Radius);
		SetComponentTickEnabled(true);
		return true;
	}
	case ECanergyAbilityAction::SuperSpray:
	{
		if (UCanergyToyWeaponControllerComponent* Weapon = Character->FindComponentByClass<UCanergyToyWeaponControllerComponent>())
		{
			Weapon->SetAbilityFireRateMultiplier(FMath::Clamp(Definition->Magnitude, 0.25f, 0.9f));
			World->GetTimerManager().ClearTimer(TemporaryAbilityTimer);
			World->GetTimerManager().SetTimer(TemporaryAbilityTimer, this,
				&UCanergyAbilityControllerComponent::ResetSuperSprayAuthorityOnly,
				FMath::Max(0.5f, Definition->DurationSeconds), false);
			return true;
		}
		ShowAbilityFeedback(FText::FromString(TEXT("喷绘器还没有准备好")));
		return false;
	}
	default:
		return false;
	}
}

void UCanergyAbilityControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AActor* Pet = ActivePet.Get();
	ACharacter* Character = GetOwnerCharacter();
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Pet || !Character)
	{
		if (!Pet || !Character) SetComponentTickEnabled(false);
		return;
	}
	if (GetServerWorldTime() >= ActivePetEndTime)
	{
		Pet->Destroy();
		ActivePet.Reset();
		SetComponentTickEnabled(false);
		return;
	}

	AActor* Target = nullptr;
	float NearestDistanceSquared = FMath::Square(ActivePetChaseRadius);
	TArray<AActor*> Collectibles;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("CarnivalCollectible"), Collectibles);
	for (AActor* Collectible : Collectibles)
	{
		if (!IsValid(Collectible)) continue;
		const float DistanceSquared = FVector::DistSquared(Pet->GetActorLocation(), Collectible->GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			Target = Collectible;
		}
	}

	FVector ViewLocation = Character->GetPawnViewLocation();
	FRotator PetViewRotation = Character->GetActorRotation();
	if (AController* Controller = Character->GetController())
	{
		Controller->GetPlayerViewPoint(ViewLocation, PetViewRotation);
	}
	const FVector FollowLocation = ViewLocation + PetViewRotation.Vector() * 260.0f
		- PetViewRotation.RotateVector(FVector::RightVector) * 75.0f + FVector::UpVector * 25.0f;
	const FVector TargetLocation = Target ? Target->GetActorLocation() : FollowLocation;
	const FVector NewLocation = FMath::VInterpTo(Pet->GetActorLocation(), TargetLocation, DeltaTime, 5.0f);
	Pet->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
	const float PulseScale = 0.36f + FMath::Sin(GetServerWorldTime() * 5.0f) * 0.025f;
	Pet->SetActorScale3D(FVector(PulseScale));
	if (Target && FVector::DistSquared(Pet->GetActorLocation(), TargetLocation) <= FMath::Square(110.0f))
	{
		Target->Destroy();
		if (UCanergyJoyScoreComponent* Joy = Character->FindComponentByClass<UCanergyJoyScoreComponent>())
		{
			Joy->AwardJoyAuthorityOnly(ECanergyJoyAction::Collectible);
		}
		ShowAbilityFeedback(FText::FromString(TEXT("彩能伙伴叼回一颗欢乐彩泡！")));
	}
}

void UCanergyAbilityControllerComponent::ResetSuperSprayAuthorityOnly()
{
	if (ACharacter* Character = GetOwnerCharacter())
	{
		if (UCanergyToyWeaponControllerComponent* Weapon = Character->FindComponentByClass<UCanergyToyWeaponControllerComponent>())
		{
			Weapon->SetAbilityFireRateMultiplier(1.0f);
		}
	}
}

void UCanergyAbilityControllerComponent::UpdateSelectedCooldownReplication()
{
	SelectedAbilityReadyAtServerTime = AbilityReadyAtServerTimes.IsValidIndex(ActiveAbilityIndex)
		? AbilityReadyAtServerTimes[ActiveAbilityIndex] : 0.0f;
	if (AActor* Owner = GetOwner()) Owner->ForceNetUpdate();
	OnRep_AbilitySelection();
}

void UCanergyAbilityControllerComponent::OnRep_AbilitySelection()
{
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		const float Remaining = GetSelectedAbilityCooldownRemaining();
		const FText Status = Remaining > 0.0f
			? FText::Format(FText::FromString(TEXT("技能选择 · {0} · 冷却 {1} 秒")),
				GetSelectedAbilityName(), FText::AsNumber(FMath::CeilToInt(Remaining)))
			: FText::Format(FText::FromString(TEXT("技能选择 · {0} · 按 F 发动")), GetSelectedAbilityName());
		OnAbilityFeedback.Broadcast(Status);
	}
}

void UCanergyAbilityControllerComponent::ShowAbilityFeedback(const FText& Message)
{
	AActor* Owner = GetOwner();
	const APawn* Pawn = Cast<APawn>(Owner);
	if (Owner && Owner->HasAuthority() && Pawn && Pawn->IsLocallyControlled())
	{
		OnAbilityFeedback.Broadcast(Message);
	}
	else if (Owner && Owner->HasAuthority())
	{
		ClientShowAbilityFeedback(Message);
	}
}

void UCanergyAbilityControllerComponent::ClientShowAbilityFeedback_Implementation(const FText& Message)
{
	OnAbilityFeedback.Broadcast(Message);
}

void UCanergyAbilityControllerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(TemporaryAbilityTimer);
	if (AActor* Pet = ActivePet.Get()) Pet->Destroy();
	Super::EndPlay(EndPlayReason);
}

void UCanergyAbilityControllerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCanergyAbilityControllerComponent, ActiveAbilityIndex);
	DOREPLIFETIME(UCanergyAbilityControllerComponent, SelectedAbilityReadyAtServerTime);
}

UCanergyToyWeaponControllerComponent::UCanergyToyWeaponControllerComponent()
{
	SetIsReplicatedByDefault(true);
}

void UCanergyToyWeaponControllerComponent::BeginPlay()
{
	Super::BeginPlay();
	if (RuntimePrototypeDefinitions.IsEmpty())
	{
		static const TCHAR* WeaponNames[] =
		{
			TEXT("棱花喷绘器"), TEXT("牵引风筒"), TEXT("彩能缝线"),
			TEXT("轻云泡弹"), TEXT("回旋弹簧枪"), TEXT("狂欢泡泡炮")
		};
		static const ECanergyWeaponDeliveryMode DeliveryModes[] =
		{
			ECanergyWeaponDeliveryMode::SprayFan, ECanergyWeaponDeliveryMode::AttractorCone,
			ECanergyWeaponDeliveryMode::SurfaceLink, ECanergyWeaponDeliveryMode::FloatProjectile,
			ECanergyWeaponDeliveryMode::RicochetProjectile, ECanergyWeaponDeliveryMode::SeekingAreaBurst
		};
		static const ECanergySurfaceKind SurfaceKinds[] =
		{
			ECanergySurfaceKind::Liquid, ECanergySurfaceKind::Sticky, ECanergySurfaceKind::Conductive,
			ECanergySurfaceKind::Float, ECanergySurfaceKind::Bounce, ECanergySurfaceKind::Mirror
		};
		static const float FireIntervals[] = { 0.28f, 0.55f, 0.8f, 0.72f, 0.42f, 1.1f };
		static const float Forces[] = { 420.0f, 560.0f, 0.0f, 520.0f, 640.0f, 360.0f };
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(WeaponNames); ++Index)
		{
			UCanergyWeaponDefinition* Prototype = NewObject<UCanergyWeaponDefinition>(this,
				FName(*FString::Printf(TEXT("PrototypeWeapon_%d"), Index)));
			Prototype->WeaponId = FName(*FString::Printf(TEXT("toy-%d"), Index + 1));
			Prototype->DisplayName = FText::FromString(WeaponNames[Index]);
			Prototype->DeliveryMode = DeliveryModes[Index];
			Prototype->PaintedSurface = SurfaceKinds[Index];
			Prototype->bAllowSurfaceModeOverride = Index == 0;
			Prototype->FireIntervalSeconds = FireIntervals[Index];
			Prototype->Range = 2400.0f;
			Prototype->InteractionRadius = Index == 5 ? 430.0f : (Index == 2 ? 620.0f : 160.0f);
			Prototype->FanSpreadDegrees = Index == 0 ? 7.0f : 0.0f;
			Prototype->ForceMagnitude = Forces[Index];
			if (Index == 1) Prototype->OnHitEffect.Kind = ECanergyInteractionKind::Pull;
			if (Index == 3) Prototype->OnHitEffect.Kind = ECanergyInteractionKind::Lightened;
			if (Index == 5) Prototype->OnHitEffect.Kind = ECanergyInteractionKind::Bubbled;
			Prototype->bHasBloomSpecial = Index == 0;
			Prototype->OnHitEffect.DurationSeconds = Index == 5 ? 3.0f : 0.45f;
			Prototype->OnHitEffect.Magnitude = Forces[Index];
			RuntimePrototypeDefinitions.Add(Prototype);
		}
	}
	if (const UCanergyWeaponDefinition* InitialWeapon = GetWeaponDefinition())
	{
		SelectedPaintSurface = InitialWeapon->PaintedSurface;
	}
}

void UCanergyToyWeaponControllerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(FireTimer);
	Super::EndPlay(EndPlayReason);
}

UCanergyWeaponDefinition* UCanergyToyWeaponControllerComponent::GetWeaponDefinition() const
{
	const TArray<TObjectPtr<UCanergyWeaponDefinition>>& Definitions = WeaponLoadout.IsEmpty()
		? RuntimePrototypeDefinitions : WeaponLoadout;
	return Definitions.IsValidIndex(ActiveWeaponIndex) ? Definitions[ActiveWeaponIndex].Get() : nullptr;
}

void UCanergyToyWeaponControllerComponent::SetWeaponDefinition(UCanergyWeaponDefinition* NewDefinition)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		WeaponLoadout.Reset();
		if (NewDefinition) WeaponLoadout.Add(NewDefinition);
		ActiveWeaponIndex = 0;
		SelectedPaintSurface = NewDefinition ? NewDefinition->PaintedSurface : ECanergySurfaceKind::Liquid;
	}
}

FText UCanergyToyWeaponControllerComponent::CycleWeapon()
{
	const TArray<TObjectPtr<UCanergyWeaponDefinition>>& Definitions = WeaponLoadout.IsEmpty()
		? RuntimePrototypeDefinitions : WeaponLoadout;
	if (Definitions.IsEmpty()) return FText::GetEmpty();
	const int32 NextIndex = (ActiveWeaponIndex + 1) % Definitions.Num();
	if (GetOwner() && GetOwner()->HasAuthority()) CycleWeaponAuthorityOnly();
	else ServerCycleWeapon();
	return Definitions.IsValidIndex(NextIndex) && Definitions[NextIndex]
		? Definitions[NextIndex]->DisplayName : FText::GetEmpty();
}

void UCanergyToyWeaponControllerComponent::ServerCycleWeapon_Implementation()
{
	CycleWeaponAuthorityOnly();
}

void UCanergyToyWeaponControllerComponent::CycleWeaponAuthorityOnly()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	const int32 DefinitionCount = WeaponLoadout.IsEmpty()
		? RuntimePrototypeDefinitions.Num() : WeaponLoadout.Num();
	if (DefinitionCount <= 0) return;
	ActiveWeaponIndex = (ActiveWeaponIndex + 1) % DefinitionCount;
	GetOwner()->ForceNetUpdate();
}

FText UCanergyToyWeaponControllerComponent::CyclePaintSurface()
{
	const ECanergySurfaceKind NextSurface = CainengGameRules::GetNextSpraySurfaceKind(SelectedPaintSurface);
	if (GetOwner() && GetOwner()->HasAuthority()) CyclePaintSurfaceAuthorityOnly();
	else ServerCyclePaintSurface();

	switch (NextSurface)
	{
	case ECanergySurfaceKind::Bounce: return FText::FromString(TEXT("橙弹跳"));
	case ECanergySurfaceKind::Float: return FText::FromString(TEXT("洋红飘浮"));
	default: return FText::FromString(TEXT("青液态"));
	}
}

void UCanergyToyWeaponControllerComponent::ServerCyclePaintSurface_Implementation()
{
	CyclePaintSurfaceAuthorityOnly();
}

void UCanergyToyWeaponControllerComponent::CyclePaintSurfaceAuthorityOnly()
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	SelectedPaintSurface = CainengGameRules::GetNextSpraySurfaceKind(SelectedPaintSurface);
	GetOwner()->ForceNetUpdate();
}

ECanergySurfaceKind UCanergyToyWeaponControllerComponent::GetEffectivePaintedSurface() const
{
	const UCanergyWeaponDefinition* Definition = GetWeaponDefinition();
	if (!Definition) return ECanergySurfaceKind::None;
	return Definition->bAllowSurfaceModeOverride ? SelectedPaintSurface : Definition->PaintedSurface;
}

void UCanergyToyWeaponControllerComponent::SetFireRateMultiplier(float NewMultiplier)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	FireRateMultiplier = FMath::Clamp(NewMultiplier, 0.25f, 2.0f);
	if (!bWantsToFire) return;
	GetWorld()->GetTimerManager().ClearTimer(FireTimer);
	const float Interval = GetEffectiveFireInterval();
	GetWorld()->GetTimerManager().SetTimer(FireTimer, this,
		&UCanergyToyWeaponControllerComponent::FireTimerTick, Interval, true, Interval);
}

void UCanergyToyWeaponControllerComponent::SetAbilityFireRateMultiplier(float NewMultiplier)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	AbilityFireRateMultiplier = FMath::Clamp(NewMultiplier, 0.25f, 1.0f);
	if (!bWantsToFire) return;
	GetWorld()->GetTimerManager().ClearTimer(FireTimer);
	const float Interval = GetEffectiveFireInterval();
	GetWorld()->GetTimerManager().SetTimer(FireTimer, this,
		&UCanergyToyWeaponControllerComponent::FireTimerTick, Interval, true, Interval);
}

float UCanergyToyWeaponControllerComponent::GetEffectiveFireInterval() const
{
	const UCanergyWeaponDefinition* Definition = GetWeaponDefinition();
	const float BaseInterval = Definition ? FMath::Clamp(Definition->FireIntervalSeconds, 0.05f, 2.0f) : 0.32f;
	const float CombinedMultiplier = FMath::Clamp(FireRateMultiplier * AbilityFireRateMultiplier, 0.15f, 2.0f);
	return BaseInterval * CombinedMultiplier;
}

void UCanergyToyWeaponControllerComponent::StartFiring()
{
	if (!GetOwner()) return;
	if (GetOwner()->HasAuthority()) SetFiringAuthorityOnly(true);
	else ServerSetFiring(true);
}

void UCanergyToyWeaponControllerComponent::StopFiring()
{
	if (!GetOwner()) return;
	if (GetOwner()->HasAuthority()) SetFiringAuthorityOnly(false);
	else ServerSetFiring(false);
}

void UCanergyToyWeaponControllerComponent::ServerSetFiring_Implementation(bool bEnable)
{
	SetFiringAuthorityOnly(bEnable);
}

void UCanergyToyWeaponControllerComponent::SetFiringAuthorityOnly(bool bEnable)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bWantsToFire == bEnable) return;
	const auto* Bubble = GetOwner()->FindComponentByClass<UCanergyBubbleRespawnComponent>();
	if (bEnable && Bubble && Bubble->IsBubbled()) return;
	bWantsToFire = bEnable;
	if (!bWantsToFire)
	{
		bShowNextShotFeedback = false;
		GetWorld()->GetTimerManager().ClearTimer(FireTimer);
		return;
	}

	bShowNextShotFeedback = true;
	FireOnceAuthorityOnly();
	const float Interval = GetEffectiveFireInterval();
	GetWorld()->GetTimerManager().SetTimer(FireTimer, this,
		&UCanergyToyWeaponControllerComponent::FireTimerTick, Interval, true, Interval);
}

void UCanergyToyWeaponControllerComponent::FireTimerTick()
{
	if (bWantsToFire) FireOnceAuthorityOnly();
}

bool UCanergyToyWeaponControllerComponent::FireOnceAuthorityOnly()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	const UCanergyWeaponDefinition* Definition = GetWeaponDefinition();
	if (!Owner || !Owner->HasAuthority() || !World || !Definition) return false;
	const auto* Bubble = Owner->FindComponentByClass<UCanergyBubbleRespawnComponent>();
	if (Bubble && Bubble->IsBubbled()) return false;
	const bool bShouldReportShot = bShowNextShotFeedback;
	bShowNextShotFeedback = false;

	FVector TraceStart;
	FRotator ViewRotation;
	APawn* OwnerPawn = Cast<APawn>(Owner);
	if (AController* Controller = OwnerPawn ? OwnerPawn->GetController() : nullptr)
	{
		Controller->GetPlayerViewPoint(TraceStart, ViewRotation);
	}
	else
	{
		Owner->GetActorEyesViewPoint(TraceStart, ViewRotation);
	}
	const FVector ShotDirection = ViewRotation.Vector().GetSafeNormal();
	const float MaxRange = FMath::Clamp(Definition->Range, 100.0f, 10000.0f);
	const FVector TraceEnd = TraceStart + ShotDirection * MaxRange;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CanergyToyWeapon), false, Owner);

	if (Definition->DeliveryMode == ECanergyWeaponDeliveryMode::SprayFan)
	{
		const FVector CameraRight = ViewRotation.RotateVector(FVector::RightVector).GetSafeNormal();
		const FVector CameraUp = ViewRotation.RotateVector(FVector::UpVector).GetSafeNormal();
		const float FanOffset = FMath::Tan(FMath::DegreesToRadians(
			FMath::Clamp(Definition->FanSpreadDegrees, 0.0f, 18.0f) * 0.5f));
		const FVector DirectionOffsets[] =
		{
			FVector::ZeroVector,
			CameraRight * FanOffset,
			-CameraRight * FanOffset,
			CameraUp * FanOffset,
			-CameraUp * FanOffset
		};
		TSet<AActor*> ResolvedTargets;
		int32 HitCount = 0;
		int32 PaintableHitCount = 0;
		int32 ChangedSurfaceCount = 0;
		for (const FVector& Offset : DirectionOffsets)
		{
			const FVector FanDirection = (ShotDirection + Offset).GetSafeNormal();
			FHitResult FanHit;
			if (!World->SweepSingleByChannel(FanHit, TraceStart, TraceStart + FanDirection * MaxRange,
				FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(24.0f), QueryParams)) continue;

			AActor* HitActor = FanHit.GetActor();
			if (!HitActor || ResolvedTargets.Contains(HitActor)) continue;
			ResolvedTargets.Add(HitActor);
			if (HitActor->ActorHasTag(FName("Paintable"))
				|| HitActor->FindComponentByClass<UCanergyPaintableSurfaceComponent>())
			{
				++PaintableHitCount;
			}
			if (ResolveHitAuthorityOnly(FanHit, FanDirection)) ++ChangedSurfaceCount;
			++HitCount;
		}
		if (bShouldReportShot)
		{
			ReportShotFeedback(ChangedSurfaceCount > 0
				? FText::FromString(FString::Printf(TEXT("棱花喷绘更新了 %d 个移动表面"), ChangedSurfaceCount))
				: PaintableHitCount > 0
					? FText::FromString(TEXT("命中彩能面 · 当前已经是这种喷绘模式"))
				: HitCount > 0
					? FText::FromString(TEXT("棱花喷绘命中场景 · 瞄准可喷绘彩能面铺路"))
					: FText::FromString(TEXT("棱花喷绘未命中 · 靠近并瞄准场景表面再试")));
		}
		return HitCount > 0;
	}

	if (Definition->DeliveryMode == ECanergyWeaponDeliveryMode::RicochetProjectile)
	{
		FVector SegmentStart = TraceStart;
		FVector SegmentDirection = ShotDirection;
		float RemainingRange = MaxRange;
		int32 ResolvedHits = 0;
		int32 ChangedSurfaceCount = 0;
		for (int32 Bounce = 0; Bounce < 3 && RemainingRange > 100.0f; ++Bounce)
		{
			FHitResult Hit;
			const FVector SegmentEnd = SegmentStart + SegmentDirection * RemainingRange;
			if (!World->SweepSingleByChannel(Hit, SegmentStart, SegmentEnd, FQuat::Identity, ECC_Visibility,
				FCollisionShape::MakeSphere(10.0f), QueryParams)) break;

			if (ResolveHitAuthorityOnly(Hit, SegmentDirection)) ++ChangedSurfaceCount;
			++ResolvedHits;
			RemainingRange -= Hit.Distance;
			QueryParams.AddIgnoredActor(Hit.GetActor());
			SegmentDirection = FMath::GetReflectionVector(SegmentDirection, Hit.ImpactNormal).GetSafeNormal();
			SegmentStart = Hit.ImpactPoint + SegmentDirection * 8.0f;
		}
		if (bShouldReportShot)
		{
			ReportShotFeedback(ChangedSurfaceCount > 0
				? FText::FromString(FString::Printf(TEXT("回旋纸刃更新了 %d 个彩能面"), ChangedSurfaceCount))
				: ResolvedHits > 0
					? FText::FromString(TEXT("回旋纸刃命中 · 弹道已折返"))
					: FText::FromString(TEXT("回旋纸刃未命中 · 找墙角试试折返")));
		}
		return ResolvedHits > 0;
	}

	const float SweepRadius = Definition->DeliveryMode == ECanergyWeaponDeliveryMode::SprayFan ? 36.0f : 12.0f;
	FHitResult Hit;
	const bool bHit = World->SweepSingleByChannel(Hit, TraceStart, TraceEnd, FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(SweepRadius), QueryParams);
	const FVector EffectCenter = bHit ? Hit.ImpactPoint : TraceEnd;
	bool bConeAffected = false;
	bool bSurfaceChanged = false;

	if (Definition->DeliveryMode == ECanergyWeaponDeliveryMode::SeekingAreaBurst)
	{
		TArray<FOverlapResult> Overlaps;
		World->OverlapMultiByChannel(Overlaps, EffectCenter, FQuat::Identity, ECC_Pawn,
			FCollisionShape::MakeSphere(FMath::Clamp(Definition->InteractionRadius, 50.0f, 900.0f)), QueryParams);
		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (AActor* Target = Overlap.GetActor())
			{
				if (Target->ActorHasTag(FName("Paintable")))
				{
					bSurfaceChanged |= PaintSurfaceAuthorityOnly(Target, GetEffectivePaintedSurface(), false);
				}
				ApplyInteractionAuthorityOnly(Target, (Target->GetActorLocation() - TraceStart).GetSafeNormal(), Definition->ForceMagnitude);
			}
		}
	}
	else if (Definition->DeliveryMode == ECanergyWeaponDeliveryMode::AttractorCone)
	{
		TArray<FOverlapResult> Overlaps;
		const float ConeLength = FMath::Min(MaxRange, 1150.0f);
		FCollisionObjectQueryParams AttractorObjects;
		AttractorObjects.AddObjectTypesToQuery(ECC_Pawn);
		AttractorObjects.AddObjectTypesToQuery(ECC_PhysicsBody);
		World->OverlapMultiByObjectType(Overlaps, TraceStart + ShotDirection * (ConeLength * 0.5f),
			FQuat::Identity, AttractorObjects, FCollisionShape::MakeSphere(ConeLength * 0.58f), QueryParams);
		for (const FOverlapResult& Overlap : Overlaps)
		{
			AActor* Target = Overlap.GetActor();
			if (!Target || (bHit && Target == Hit.GetActor())) continue;
			const FVector Offset = Target->GetActorLocation() - TraceStart;
			const float ForwardDistance = FVector::DotProduct(Offset, ShotDirection);
			const float LateralDistance = (Offset - ShotDirection * ForwardDistance).Size();
			if (ForwardDistance < 100.0f || ForwardDistance > ConeLength
				|| LateralDistance > 120.0f + ForwardDistance * 0.38f) continue;
			const UPrimitiveComponent* PhysicsBody = Cast<UPrimitiveComponent>(Target->GetRootComponent());
			if (!Cast<ACharacter>(Target) && (!PhysicsBody || !PhysicsBody->IsSimulatingPhysics())) continue;
			ApplyInteractionAuthorityOnly(Target, ShotDirection, Definition->ForceMagnitude);
			bConeAffected = true;
		}
		if (bHit) bSurfaceChanged |= ResolveHitAuthorityOnly(Hit, ShotDirection);
	}
	else if (bHit)
	{
		bSurfaceChanged |= ResolveHitAuthorityOnly(Hit, ShotDirection);
		if (Definition->DeliveryMode == ECanergyWeaponDeliveryMode::SurfaceLink)
		{
			TArray<FOverlapResult> NearbySurfaces;
			World->OverlapMultiByChannel(NearbySurfaces, Hit.ImpactPoint, FQuat::Identity, ECC_Visibility,
				FCollisionShape::MakeSphere(FMath::Clamp(Definition->InteractionRadius, 50.0f, 900.0f)), QueryParams);
			int32 LinkedSurfaceCount = 0;
			for (const FOverlapResult& Nearby : NearbySurfaces)
			{
				AActor* SurfaceActor = Nearby.GetActor();
				if (!SurfaceActor || SurfaceActor == Hit.GetActor() || !SurfaceActor->ActorHasTag(FName("Paintable"))) continue;
				bSurfaceChanged |= PaintSurfaceAuthorityOnly(SurfaceActor, GetEffectivePaintedSurface(), true);
				if (++LinkedSurfaceCount >= 4) break;
			}
		}
	}

	if (Definition->DeliveryMode == ECanergyWeaponDeliveryMode::FloatProjectile && bHit)
	{
		if (ACharacter* Character = Cast<ACharacter>(Hit.GetActor()))
		{
			Character->LaunchCharacter(FVector::UpVector * FMath::Max(350.0f, Definition->ForceMagnitude), false, true);
		}
	}
	if (bShouldReportShot)
	{
		const AActor* HitActor = bHit ? Hit.GetActor() : nullptr;
		const bool bHitPaintable = HitActor && (HitActor->ActorHasTag(FName("Paintable"))
			|| HitActor->FindComponentByClass<UCanergyPaintableSurfaceComponent>());
		const bool bAnyInteraction = bHit || bConeAffected
			|| Definition->DeliveryMode == ECanergyWeaponDeliveryMode::SeekingAreaBurst;
		ReportShotFeedback(bSurfaceChanged
			? FText::FromString(TEXT("彩能表面已改变 · 新移动效果生效"))
			: bHitPaintable
				? FText::FromString(TEXT("命中彩能面 · 当前已经是这种喷绘模式"))
			: bAnyInteraction
				? FText::Format(FText::FromString(TEXT("{0} · 玩具互动已发动")), Definition->DisplayName)
				: FText::FromString(TEXT("没有命中互动目标 · 靠近目标或调整视角")));
		}
	return bHit || bConeAffected || Definition->DeliveryMode == ECanergyWeaponDeliveryMode::SeekingAreaBurst;
}

void UCanergyToyWeaponControllerComponent::ReportShotFeedback(const FText& Feedback)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsPlayerControlled()) return;

	if (OwnerPawn->IsLocallyControlled() || OwnerPawn->GetNetMode() == NM_Standalone)
	{
		OnWeaponFeedback.Broadcast(Feedback);
	}
	else
	{
		ClientShowWeaponFeedback(Feedback);
	}
}

void UCanergyToyWeaponControllerComponent::ClientShowWeaponFeedback_Implementation(const FText& Feedback)
{
	OnWeaponFeedback.Broadcast(Feedback);
}

bool UCanergyToyWeaponControllerComponent::ResolveHitAuthorityOnly(
	const FHitResult& Hit, const FVector& ShotDirection, bool bAwardSurfaceJoy)
{
	const UCanergyWeaponDefinition* Definition = GetWeaponDefinition();
	AActor* Target = Hit.GetActor();
	if (!Definition || !Target) return false;

	bool bSurfaceChanged = false;
	if (Target->ActorHasTag(FName("Paintable"))
		|| Target->FindComponentByClass<UCanergyPaintableSurfaceComponent>())
	{
		bSurfaceChanged = PaintSurfaceAuthorityOnly(Target, GetEffectivePaintedSurface(), bAwardSurfaceJoy);
	}
	ApplyInteractionAuthorityOnly(Target, ShotDirection, Definition->ForceMagnitude);
	return bSurfaceChanged;
}

bool UCanergyToyWeaponControllerComponent::PaintSurfaceAuthorityOnly(
	AActor* Target, ECanergySurfaceKind SurfaceKind, bool bAwardJoy)
{
	if (!Target || !Target->HasAuthority() || SurfaceKind == ECanergySurfaceKind::None) return false;
	UCanergyPaintableSurfaceComponent* Surface = Target->FindComponentByClass<UCanergyPaintableSurfaceComponent>();
	if (!Surface)
	{
		if (!Target->ActorHasTag(FName("Paintable"))) return false;
		Surface = NewObject<UCanergyPaintableSurfaceComponent>(Target);
		Target->AddInstanceComponent(Surface);
		Surface->RegisterComponent();
	}
	const bool bWasUnpainted = !Surface->IsPainted();
	const bool bSurfaceChanged = Surface->ApplySurfaceKindAuthorityOnly(SurfaceKind);
	if (bSurfaceChanged && bAwardJoy && bWasUnpainted)
	{
		if (ACharacter* Shooter = Cast<ACharacter>(GetOwner()))
		{
			if (UCanergyJoyScoreComponent* Joy = Shooter->FindComponentByClass<UCanergyJoyScoreComponent>())
			{
				Joy->AwardJoyAuthorityOnly(ECanergyJoyAction::SurfaceTrail);
			}
		}
	}
	if (AShooterGameMode* GameMode = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		if (Target->ActorHasTag(FName("PublicObjective_Target")))
		{
			GameMode->RegisterPublicObjectiveHit(Target, GetOwner());
		}
	}
	if (Target->ActorHasTag(FName("CarnivalCollectible")))
	{
		if (ACharacter* Shooter = Cast<ACharacter>(GetOwner()))
		{
			if (UCanergyJoyScoreComponent* Joy = Shooter->FindComponentByClass<UCanergyJoyScoreComponent>())
			{
				Joy->AwardJoyAuthorityOnly(ECanergyJoyAction::Collectible);
			}
		}
		Target->Destroy();
	}
	return bSurfaceChanged;
}

void UCanergyToyWeaponControllerComponent::ApplyInteractionAuthorityOnly(
	AActor* Target, const FVector& ShotDirection, float ForceMagnitude)
{
	const UCanergyWeaponDefinition* Definition = GetWeaponDefinition();
	if (!Definition || !Target || Target == GetOwner()) return;

	FCanergyInteractionEffectSpec Effect = Definition->OnHitEffect;
	if (Effect.Kind == ECanergyInteractionKind::None)
	{
		switch (Definition->DeliveryMode)
		{
		case ECanergyWeaponDeliveryMode::AttractorCone: Effect.Kind = ECanergyInteractionKind::Pull; break;
		case ECanergyWeaponDeliveryMode::FloatProjectile: Effect.Kind = ECanergyInteractionKind::Lightened; break;
		case ECanergyWeaponDeliveryMode::SeekingAreaBurst: Effect.Kind = ECanergyInteractionKind::Bubbled; break;
		case ECanergyWeaponDeliveryMode::SurfaceLink: return;
		default: Effect.Kind = ECanergyInteractionKind::Knockback; break;
		}
		Effect.DurationSeconds = 0.35f;
		Effect.Magnitude = ForceMagnitude;
		Effect.StackPolicy = ECanergyEffectStackPolicy::RefreshDuration;
		Effect.MaxStacks = 1;
	}

	ACharacter* Character = Cast<ACharacter>(Target);
	if (!Character)
	{
		UPrimitiveComponent* Body = Cast<UPrimitiveComponent>(Target->GetRootComponent());
		if (!Body || !Body->IsSimulatingPhysics()) return;

		const float SpeedChange = FMath::Clamp(ForceMagnitude, 0.0f, 700.0f);
		FVector Direction = ShotDirection.GetSafeNormal();
		if (Effect.Kind == ECanergyInteractionKind::Pull)
		{
			Direction = (GetOwner()->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal();
		}
		else if (Effect.Kind == ECanergyInteractionKind::Lightened)
		{
			Direction = (FVector::UpVector * 0.8f + ShotDirection.GetSafeNormal() * 0.2f).GetSafeNormal();
		}
		else if (Effect.Kind != ECanergyInteractionKind::Knockback)
		{
			return;
		}

		const FVector NewVelocity = (Body->GetPhysicsLinearVelocity() + Direction * SpeedChange)
			.GetClampedToMaxSize(900.0f);
		Body->SetPhysicsLinearVelocity(NewVelocity);
		if (Target->ActorHasTag(TEXT("CanergyCarryCore")))
		{
			UE_LOG(LogTemp, Display, TEXT("CANERGY_OBJECTIVE_TOOL weapon=%s coreSpeed=%.1f"),
				*Definition->DisplayName.ToString(), NewVelocity.Size());
		}
		return;
	}

	if (UCanergyInteractionComponent* Interaction = Character->FindComponentByClass<UCanergyInteractionComponent>())
	{
		Interaction->ApplyEffectAuthorityOnly(Effect);
	}
	if (auto* Mode = GetWorld()->GetAuthGameMode<AShooterGameMode>())
		if (auto* Carry = Mode->GetCarryPrototype())
			Carry->HandleWeaponHit(Cast<AShooterCharacter>(Character), Cast<AShooterCharacter>(GetOwner()));

	const float Impulse = FMath::Clamp(ForceMagnitude, 0.0f, 1800.0f);
	if (Effect.Kind == ECanergyInteractionKind::Knockback)
	{
		Character->LaunchCharacter(ShotDirection.GetSafeNormal() * Impulse + FVector::UpVector * (Impulse * 0.18f), true, true);
	}
	else if (Effect.Kind == ECanergyInteractionKind::Pull)
	{
		const FVector PullDirection = (GetOwner()->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal();
		Character->LaunchCharacter(PullDirection * Impulse + FVector::UpVector * (Impulse * 0.08f), true, true);
	}
	else if (Effect.Kind == ECanergyInteractionKind::Bubbled)
	{
		if (UCanergyBubbleRespawnComponent* Bubble = Character->FindComponentByClass<UCanergyBubbleRespawnComponent>())
		{
			Bubble->BeginBubbleAuthorityOnly();
		}
	}
}

void UCanergyToyWeaponControllerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCanergyToyWeaponControllerComponent, ActiveWeaponIndex);
	DOREPLIFETIME(UCanergyToyWeaponControllerComponent, SelectedPaintSurface);
}

void ACanergyMatchGameState::SetCanergyMatchStateAuthorityOnly(const FCanergyMatchRuntimeState& NewState)
{
	if (!HasAuthority()) return;
	const bool bImportantTransition = CanergyMatchState.Phase != NewState.Phase
		|| CanergyMatchState.ActiveObjective != NewState.ActiveObjective
		|| CanergyMatchState.ActiveCarnivalEvent != NewState.ActiveCarnivalEvent
		|| CanergyMatchState.bPublicObjectiveResolved != NewState.bPublicObjectiveResolved
		|| FMath::FloorToInt(CanergyMatchState.CarnivalMeter) != FMath::FloorToInt(NewState.CarnivalMeter);
	CanergyMatchState = NewState;
	if (bImportantTransition) ForceNetUpdate();
}

void ACanergyMatchGameState::OnRep_CanergyMatchState()
{
	OnCanergyMatchStateChanged.Broadcast(CanergyMatchState);
}

void ACanergyMatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACanergyMatchGameState, CanergyMatchState);
}
