// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayTypes.h"
#include "CanergyRuntimeComponents.generated.h"

class UCanergyCharacterGameplayDefinition;
class UCanergyCharacterVisualProfile;
class UCanergyAbilityDefinition;
class UCanergyWeaponDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCanergyGameplaySignal);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCanergyJoyScoreChanged, int32, NewScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCanergySurfaceKindChanged, ECanergySurfaceKind, NewSurfaceKind);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCanergyMatchStateChanged, FCanergyMatchRuntimeState, NewMatchState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCanergyAbilityFeedback, FText, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCanergyWeaponFeedback, FText, Message);

USTRUCT(BlueprintType)
struct FCanergyBubbleRespawnSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Canergy|Respawn")
	bool bIsBubbled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Canergy|Respawn")
	float TimeInBubbleSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Canergy|Respawn")
	bool bRespawnReady = false;
};

/** Applies optional data definitions to any ACharacter without coupling gameplay to its model asset. */
UCLASS(ClassGroup = (Canergy), meta = (BlueprintSpawnableComponent))
class TP_FIRSTPERSON_API UCanergyCharacterProfileComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCanergyCharacterProfileComponent();

	UFUNCTION(BlueprintCallable, Category = "Canergy|Character")
	bool ApplyDefinitions();

	UCanergyCharacterGameplayDefinition* GetGameplayDefinition() const { return LoadedGameplayDefinition; }
	UCanergyCharacterVisualProfile* GetVisualProfile() const { return LoadedVisualProfile; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Character")
	TSoftObjectPtr<UCanergyCharacterGameplayDefinition> GameplayDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Character")
	TSoftObjectPtr<UCanergyCharacterVisualProfile> VisualProfileOverride;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Canergy|Character")
	TObjectPtr<UCanergyCharacterGameplayDefinition> LoadedGameplayDefinition;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Canergy|Character")
	TObjectPtr<UCanergyCharacterVisualProfile> LoadedVisualProfile;

	virtual void BeginPlay() override;
};

/** Replicated gameplay state for paintable world geometry; color is presentation, kind is the rule. */
UCLASS(ClassGroup = (Canergy), meta = (BlueprintSpawnableComponent))
class TP_FIRSTPERSON_API UCanergyPaintableSurfaceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCanergyPaintableSurfaceComponent();

	UFUNCTION(BlueprintCallable, Category = "Canergy|Surface")
	bool ApplySurfaceKindAuthorityOnly(ECanergySurfaceKind NewKind);

	UFUNCTION(BlueprintCallable, Category = "Canergy|Surface")
	bool ClearSurfaceAuthorityOnly();

	UFUNCTION(BlueprintPure, Category = "Canergy|Surface")
	ECanergySurfaceKind GetSurfaceKind() const { return SurfaceKind; }

	UFUNCTION(BlueprintPure, Category = "Canergy|Surface")
	bool IsPainted() const { return SurfaceKind != ECanergySurfaceKind::None; }

	UPROPERTY(BlueprintAssignable, Category = "Canergy|Surface")
	FCanergySurfaceKindChanged OnSurfaceKindChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_SurfaceKind, BlueprintReadOnly, Category = "Canergy|Surface")
	ECanergySurfaceKind SurfaceKind = ECanergySurfaceKind::None;

	UFUNCTION()
	void OnRep_SurfaceKind();

	void UpdateSurfacePresentation();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

/** Server-owned, short-lived interaction states shared by player and bot characters. */
UCLASS(ClassGroup = (Canergy), meta = (BlueprintSpawnableComponent))
class TP_FIRSTPERSON_API UCanergyInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCanergyInteractionComponent();

	UFUNCTION(BlueprintCallable, Category = "Canergy|Interaction")
	bool ApplyEffectAuthorityOnly(const FCanergyInteractionEffectSpec& EffectSpec);

	UFUNCTION(BlueprintCallable, Category = "Canergy|Interaction")
	bool ClearEffectAuthorityOnly(ECanergyInteractionKind Kind);

	UFUNCTION(BlueprintCallable, Category = "Canergy|Interaction")
	void ClearAllEffectsAuthorityOnly();

	UFUNCTION(BlueprintPure, Category = "Canergy|Interaction")
	bool HasEffect(ECanergyInteractionKind Kind) const;

	UFUNCTION(BlueprintPure, Category = "Canergy|Interaction")
	float GetEffectRemainingSeconds(ECanergyInteractionKind Kind) const;

	const TArray<FCanergyActiveInteractionEffect>& GetActiveEffects() const { return ActiveEffects; }

	UPROPERTY(BlueprintAssignable, Category = "Canergy|Interaction")
	FCanergyGameplaySignal OnEffectsChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_ActiveEffects, BlueprintReadOnly, Category = "Canergy|Interaction")
	TArray<FCanergyActiveInteractionEffect> ActiveEffects;

	UFUNCTION()
	void OnRep_ActiveEffects();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

/** Positive Joy Score only; regular damage and elimination events are not score inputs. */
UCLASS(ClassGroup = (Canergy), meta = (BlueprintSpawnableComponent))
class TP_FIRSTPERSON_API UCanergyJoyScoreComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCanergyJoyScoreComponent();

	UFUNCTION(BlueprintCallable, Category = "Canergy|Joy")
	int32 AwardJoyAuthorityOnly(ECanergyJoyAction Action);

	UFUNCTION(BlueprintCallable, Category = "Canergy|Joy")
	void ResetJoyScoreAuthorityOnly();

	UFUNCTION(BlueprintPure, Category = "Canergy|Joy")
	int32 GetJoyScore() const { return JoyScore; }
	const TMap<ECanergyJoyAction, int32>& GetJoyContributions() const { return JoyContributions; }

	UPROPERTY(BlueprintAssignable, Category = "Canergy|Joy")
	FCanergyJoyScoreChanged OnJoyScoreChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_JoyScore, BlueprintReadOnly, Category = "Canergy|Joy")
	int32 JoyScore = 0;
	// Authority-side round ledger; final online scoreboard replication is a separate integration.
	TMap<ECanergyJoyAction, int32> JoyContributions;

	UFUNCTION()
	void OnRep_JoyScore();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

/** Three-second non-lethal bubble return window. Respawning never mutates Joy Score or permanent data. */
UCLASS(ClassGroup = (Canergy), meta = (BlueprintSpawnableComponent))
class TP_FIRSTPERSON_API UCanergyBubbleRespawnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCanergyBubbleRespawnComponent();

	UFUNCTION(BlueprintCallable, Category = "Canergy|Respawn")
	bool BeginBubbleAuthorityOnly();

	UFUNCTION(BlueprintCallable, Category = "Canergy|Respawn")
	bool CompleteRespawnAuthorityOnly();

	UFUNCTION(BlueprintPure, Category = "Canergy|Respawn")
	bool IsBubbled() const { return RespawnState.bIsBubbled; }

	UFUNCTION(BlueprintPure, Category = "Canergy|Respawn")
	bool CanRespawnNow() const { return RespawnState.bRespawnReady; }

	UFUNCTION(BlueprintPure, Category = "Canergy|Respawn")
	float GetRespawnSecondsRemaining() const;

	UPROPERTY(BlueprintAssignable, Category = "Canergy|Respawn")
	FCanergyGameplaySignal OnBubbleStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Canergy|Respawn")
	FCanergyGameplaySignal OnRespawnReady;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_RespawnState, BlueprintReadOnly, Category = "Canergy|Respawn")
	FCanergyBubbleRespawnSnapshot RespawnState;
	FTransform SafeRespawnTransform = FTransform::Identity;
	uint8 SavedMovementMode = 0;
	uint8 SavedCustomMovementMode = 0;
	TEnumAsByte<ECollisionEnabled::Type> SavedCapsuleCollision = ECollisionEnabled::QueryAndPhysics;
	bool bSavedActorHidden = false;
	float ReadyDisplaySeconds = 0.0f;

	UFUNCTION()
	void OnRep_RespawnState();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

/** Six reusable active skills. Character silhouettes/meshes never participate in their rules. */
UCLASS(ClassGroup = (Canergy), meta = (BlueprintSpawnableComponent))
class TP_FIRSTPERSON_API UCanergyAbilityControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCanergyAbilityControllerComponent();

	UFUNCTION(BlueprintCallable, Category = "Canergy|Ability")
	FText CycleAbility();

	UFUNCTION(BlueprintCallable, Category = "Canergy|Ability")
	bool ActivateSelectedAbility();

	UFUNCTION(BlueprintPure, Category = "Canergy|Ability")
	FText GetSelectedAbilityName() const;

	UFUNCTION(BlueprintPure, Category = "Canergy|Ability")
	float GetSelectedAbilityCooldownRemaining() const;

	UPROPERTY(BlueprintAssignable, Category = "Canergy|Ability")
	FCanergyAbilityFeedback OnAbilityFeedback;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Ability")
	TArray<TObjectPtr<UCanergyAbilityDefinition>> AbilityLoadout;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCanergyAbilityDefinition>> RuntimePrototypeDefinitions;

	UPROPERTY(ReplicatedUsing = OnRep_AbilitySelection, BlueprintReadOnly, Category = "Canergy|Ability")
	int32 ActiveAbilityIndex = 0;

	UPROPERTY(ReplicatedUsing = OnRep_AbilitySelection, BlueprintReadOnly, Category = "Canergy|Ability")
	float SelectedAbilityReadyAtServerTime = 0.0f;

	TArray<float> AbilityReadyAtServerTimes;
	TWeakObjectPtr<AActor> ActivePet;
	float ActivePetEndTime = 0.0f;
	float ActivePetChaseRadius = 1800.0f;
	FTimerHandle TemporaryAbilityTimer;

	UFUNCTION(Server, Reliable)
	void ServerCycleAbility();

	UFUNCTION(Server, Reliable)
	void ServerActivateSelectedAbility();

	UFUNCTION(Client, Reliable)
	void ClientShowAbilityFeedback(const FText& Message);

	UFUNCTION()
	void OnRep_AbilitySelection();

	void CycleAbilityAuthorityOnly();
	bool ActivateSelectedAbilityAuthorityOnly();
	bool ExecuteAbilityAuthorityOnly(const UCanergyAbilityDefinition* Definition);
	void BuildRuntimePrototypeDefinitions();
	void UpdateSelectedCooldownReplication();
	void ShowAbilityFeedback(const FText& Message);
	void ResetSuperSprayAuthorityOnly();
	float GetServerWorldTime() const;
	ACharacter* GetOwnerCharacter() const;
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

/** Data-driven, server-authoritative, nonlethal firing path shared by players and bots. */
UCLASS(ClassGroup = (Canergy), meta = (BlueprintSpawnableComponent))
class TP_FIRSTPERSON_API UCanergyToyWeaponControllerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCanergyToyWeaponControllerComponent();

	UPROPERTY(BlueprintAssignable, Category = "Canergy|Weapon")
	FCanergyWeaponFeedback OnWeaponFeedback;

	UFUNCTION(BlueprintCallable, Category = "Canergy|Weapon")
	void StartFiring();

	UFUNCTION(BlueprintCallable, Category = "Canergy|Weapon")
	void StopFiring();
	bool IsFiring() const { return bWantsToFire; }

	UFUNCTION(BlueprintCallable, Category = "Canergy|Weapon")
	void SetWeaponDefinition(UCanergyWeaponDefinition* NewDefinition);

	UFUNCTION(BlueprintCallable, Category = "Canergy|Weapon")
	FText CycleWeapon();

	UFUNCTION(BlueprintCallable, Category = "Canergy|Weapon")
	FText CyclePaintSurface();

	UFUNCTION(BlueprintCallable, Category = "Canergy|Weapon")
	void SetFireRateMultiplier(float NewMultiplier);

	UFUNCTION(BlueprintCallable, Category = "Canergy|Weapon")
	void SetAbilityFireRateMultiplier(float NewMultiplier);

	UFUNCTION(BlueprintPure, Category = "Canergy|Weapon")
	UCanergyWeaponDefinition* GetWeaponDefinition() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Weapon")
	TArray<TObjectPtr<UCanergyWeaponDefinition>> WeaponLoadout;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCanergyWeaponDefinition>> RuntimePrototypeDefinitions;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Canergy|Weapon")
	int32 ActiveWeaponIndex = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Canergy|Weapon")
	ECanergySurfaceKind SelectedPaintSurface = ECanergySurfaceKind::Liquid;

	bool bWantsToFire = false;
	bool bShowNextShotFeedback = false;
	float FireRateMultiplier = 1.0f;
	float AbilityFireRateMultiplier = 1.0f;
	FTimerHandle FireTimer;

	UFUNCTION(Server, Reliable)
	void ServerSetFiring(bool bEnable);

	UFUNCTION(Server, Reliable)
	void ServerCycleWeapon();

	UFUNCTION(Server, Reliable)
	void ServerCyclePaintSurface();

	UFUNCTION(Client, Reliable)
	void ClientShowWeaponFeedback(const FText& Feedback);

	void SetFiringAuthorityOnly(bool bEnable);
	void ReportShotFeedback(const FText& Feedback);
	void CycleWeaponAuthorityOnly();
	void CyclePaintSurfaceAuthorityOnly();
	ECanergySurfaceKind GetEffectivePaintedSurface() const;
	float GetEffectiveFireInterval() const;
	void FireTimerTick();
	bool FireOnceAuthorityOnly();
	bool ResolveHitAuthorityOnly(const FHitResult& Hit, const FVector& ShotDirection, bool bAwardSurfaceJoy = true);
	bool PaintSurfaceAuthorityOnly(AActor* Target, ECanergySurfaceKind SurfaceKind, bool bAwardJoy);
	void ApplyInteractionAuthorityOnly(AActor* Target, const FVector& ShotDirection, float ForceMagnitude);
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

/** Replicated match clock and optional objective/event state for all connected players. */
UCLASS()
class TP_FIRSTPERSON_API ACanergyMatchGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	void SetCanergyMatchStateAuthorityOnly(const FCanergyMatchRuntimeState& NewState);
	const FCanergyMatchRuntimeState& GetCanergyMatchState() const { return CanergyMatchState; }

	UPROPERTY(BlueprintAssignable, Category = "Canergy|Match")
	FCanergyMatchStateChanged OnCanergyMatchStateChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CanergyMatchState, BlueprintReadOnly, Category = "Canergy|Match")
	FCanergyMatchRuntimeState CanergyMatchState;

	UFUNCTION()
	void OnRep_CanergyMatchState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
