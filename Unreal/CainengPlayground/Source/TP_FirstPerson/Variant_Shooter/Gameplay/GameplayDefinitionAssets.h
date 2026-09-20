// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTypes.h"
#include "GameplayDefinitionAssets.generated.h"

class AActor;
class UAnimInstance;
class UAnimMontage;
class UMaterialInterface;
class USkeletalMesh;
class USoundBase;
class UStaticMesh;
class UTexture2D;
class UParticleSystem;
class UCanergyCharacterVisualProfile;
class UCanergyAbilityDefinition;
class ACanergyBounceFlower;

UENUM(BlueprintType)
enum class ECanergyWeaponDeliveryMode : uint8
{
	SprayFan UMETA(DisplayName = "扇形喷绘"),
	AttractorCone UMETA(DisplayName = "锥形吸附"),
	SurfaceLink UMETA(DisplayName = "表面缝线"),
	FloatProjectile UMETA(DisplayName = "漂浮弹体"),
	RicochetProjectile UMETA(DisplayName = "回旋弹体"),
	SeekingAreaBurst UMETA(DisplayName = "追踪范围爆发")
};

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class ECanergyWeaponAffix : uint8
{
	None = 0 UMETA(DisplayName = "无词条"),
	Bubble = 1 << 0 UMETA(DisplayName = "泡泡"),
	Refraction = 1 << 1 UMETA(DisplayName = "折射"),
	Magnetism = 1 << 2 UMETA(DisplayName = "磁吸"),
	Conduction = 1 << 3 UMETA(DisplayName = "传导"),
	Spring = 1 << 4 UMETA(DisplayName = "弹簧"),
	Mirror = 1 << 5 UMETA(DisplayName = "镜像")
};

UENUM(BlueprintType)
enum class ECanergyMovementTrait : uint8
{
	Lightfoot UMETA(DisplayName = "轻盈"),
	Heavy UMETA(DisplayName = "厚重"),
	Agile UMETA(DisplayName = "灵巧")
};

UENUM(BlueprintType)
enum class ECanergyAbilityAction : uint8
{
	Cannonball UMETA(DisplayName = "炮弹化"),
	WallHide UMETA(DisplayName = "墙内藏身"),
	SwapPosition UMETA(DisplayName = "位置交换"),
	InvertSurface UMETA(DisplayName = "彩能反转"),
	SummonPet UMETA(DisplayName = "召唤宠物"),
	SuperSpray UMETA(DisplayName = "超级喷射")
};

USTRUCT(BlueprintType)
struct FCanergyMovementTuning
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Movement", meta = (ClampMin = "100.0"))
	float GroundSpeed = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Movement", meta = (ClampMin = "10.0"))
	float CapsuleRadius = 34.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Movement", meta = (ClampMin = "20.0"))
	float CapsuleHalfHeight = 96.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AirControl = 0.55f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Movement", meta = (ClampMin = "0.0"))
	float JumpVelocity = 620.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Movement", meta = (ClampMin = "0.0"))
	float FloatDurationSeconds = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Movement", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float KnockbackResistance = 0.0f;
};

/** Meshes, skeleton-specific animation and effects live here, never in weapon gameplay rules. */
UCLASS(BlueprintType)
class TP_FIRSTPERSON_API UCanergyWeaponPresentationDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Mesh")
	TSoftObjectPtr<UStaticMesh> FirstPersonStaticMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Mesh")
	TSoftObjectPtr<USkeletalMesh> FirstPersonSkeletalMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Mesh")
	TSoftObjectPtr<UStaticMesh> ThirdPersonStaticMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Mesh")
	TSoftObjectPtr<USkeletalMesh> ThirdPersonSkeletalMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Animation")
	TSoftObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Animation")
	TSoftObjectPtr<UAnimMontage> SpecialMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Audio")
	TSoftObjectPtr<USoundBase> FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Audio")
	TSoftObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Effects")
	TSoftObjectPtr<UParticleSystem> FireEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Effects")
	TSoftObjectPtr<UParticleSystem> ImpactEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Attachment")
	FName FirstPersonSocket = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Attachment")
	FName ThirdPersonSocket = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Attachment")
	FTransform MeshRelativeTransform = FTransform::Identity;
};

/** Stable gameplay tuning. Swapping its presentation asset does not change any rules code. */
UCLASS(BlueprintType)
class TP_FIRSTPERSON_API UCanergyWeaponDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName WeaponId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Gameplay")
	ECanergyWeaponDeliveryMode DeliveryMode = ECanergyWeaponDeliveryMode::SprayFan;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Gameplay")
	ECanergySurfaceKind PaintedSurface = ECanergySurfaceKind::Liquid;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Gameplay")
	bool bAllowSurfaceModeOverride = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Gameplay")
	FCanergyInteractionEffectSpec OnHitEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Gameplay", meta = (ClampMin = "0.05", ClampMax = "2.0"))
	float FireIntervalSeconds = 0.28f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Gameplay", meta = (ClampMin = "100.0"))
	float Range = 2400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Gameplay", meta = (ClampMin = "0.0"))
	float InteractionRadius = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Gameplay", meta = (ClampMin = "0.0", ClampMax = "18.0"))
	float FanSpreadDegrees = 6.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Gameplay", meta = (ClampMin = "0.0"))
	float ForceMagnitude = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Special")
	bool bHasBloomSpecial = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Special")
	float SpecialCooldownSeconds = 8.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Special")
	float SpecialLifetimeSeconds = 8.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Special")
	float SpecialRadius = 220.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Special")
	float SpecialLaunchSpeed = 950.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Special")
	TSoftClassPtr<ACanergyBounceFlower> BloomActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Gameplay", meta = (Bitmask, BitmaskEnum = "/Script/TP_FirstPerson.ECanergyWeaponAffix"))
	int32 AffixMask = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Gameplay")
	TSoftClassPtr<AActor> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Presentation")
	TSoftObjectPtr<UCanergyWeaponPresentationDefinition> Presentation;
};

/** Gameplay values are reusable across meshes; movement traits are small preference shifts only. */
UCLASS(BlueprintType)
class TP_FIRSTPERSON_API UCanergyCharacterGameplayDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Gameplay")
	ECanergyMovementTrait MovementTrait = ECanergyMovementTrait::Agile;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Gameplay")
	FCanergyMovementTuning Movement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Gameplay")
	TArray<TSoftObjectPtr<UCanergyAbilityDefinition>> Abilities;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Character|Presentation")
	TSoftObjectPtr<UCanergyCharacterVisualProfile> DefaultVisualProfile;
};

/** Character mesh/skeleton/materials can be swapped without replacing its gameplay definition. */
UCLASS(BlueprintType)
class TP_FIRSTPERSON_API UCanergyCharacterVisualProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Character")
	TSoftObjectPtr<USkeletalMesh> BodyMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Character")
	TSoftObjectPtr<USkeletalMesh> FirstPersonMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Animation")
	TSoftClassPtr<UAnimInstance> ThirdPersonAnimationClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Animation")
	TSoftClassPtr<UAnimInstance> FirstPersonAnimationClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Character")
	TArray<TSoftObjectPtr<UMaterialInterface>> MaterialOverrides;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Character")
	TArray<TSoftObjectPtr<UMaterialInterface>> FirstPersonMaterialOverrides;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Character")
	FTransform BodyMeshRelativeTransform = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Character")
	FTransform FirstPersonMeshRelativeTransform = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Character")
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Presentation|Character")
	FLinearColor AccentColor = FLinearColor::White;
};

UCLASS(BlueprintType)
class TP_FIRSTPERSON_API UCanergyAbilityDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	ECanergyAbilityAction Action = ECanergyAbilityAction::Cannonball;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = "0.0"))
	float CooldownSeconds = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = "0.0", ClampMax = "20.0"))
	float DurationSeconds = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = "0.0"))
	float Radius = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = "0.0"))
	float Magnitude = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability|Presentation")
	TSoftClassPtr<AActor> SpawnedActorClass;
};

UCLASS(BlueprintType)
class TP_FIRSTPERSON_API UCanergyPublicObjectiveDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
	ECanergyPublicObjectiveKind Kind = ECanergyPublicObjectiveKind::PaintMonument;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective", meta = (ClampMin = "10.0", ClampMax = "240.0"))
	float DurationSeconds = 90.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective", meta = (ClampMin = "0"))
	int32 JoyReward = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective", meta = (ClampMin = "0"))
	int32 CarnivalMeterReward = 10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Presentation")
	TSoftClassPtr<AActor> ObjectiveActorClass;
};

UCLASS(BlueprintType)
class TP_FIRSTPERSON_API UCanergyMatchDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match")
	FCanergyMatchRulesSettings Rules;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match", meta = (ClampMin = "2", ClampMax = "6"))
	int32 TargetPlayerCount = 6;
};

UCLASS(BlueprintType)
class TP_FIRSTPERSON_API UCanergyCarnivalEventDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carnival")
	ECanergyCarnivalEventKind Kind = ECanergyCarnivalEventKind::LowGravity;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carnival")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carnival", meta = (ClampMin = "5.0", ClampMax = "30.0"))
	float DurationSeconds = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Carnival|Presentation")
	TSoftClassPtr<AActor> EventActorClass;
};
