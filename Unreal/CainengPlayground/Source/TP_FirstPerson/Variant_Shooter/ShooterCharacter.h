// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TP_FirstPersonCharacter.h"
#include "ShooterWeaponHolder.h"
#include "Gameplay/TraversalRules.h"
#include "ShooterCharacter.generated.h"

class AShooterWeapon;
class UInputAction;
class UInputComponent;
class UPawnNoiseEmitterComponent;
class UCanergyInteractionComponent;
class UCanergyJoyScoreComponent;
class UCanergyBubbleRespawnComponent;
class UCanergyCharacterProfileComponent;
class UCanergyAbilityControllerComponent;
class UCanergyToyWeaponControllerComponent;
class UCanergyWallTraversalComponent;
class UCanergyWeaponSpecialComponent;
class UCanergyPhaseAbilityComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBulletCountUpdatedDelegate, int32, MagazineSize, int32, Bullets);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDamagedDelegate, float, LifePercent);

/**
 *  A player controllable first person shooter character
 *  Manages a weapon inventory through the IShooterWeaponHolder interface
 *  Manages health and death
 */
UCLASS(abstract)
class TP_FIRSTPERSON_API AShooterCharacter : public ATP_FirstPersonCharacter, public IShooterWeaponHolder
{
	GENERATED_BODY()
	
	/** AI Noise emitter component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPawnNoiseEmitterComponent* PawnNoiseEmitter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Canergy", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCanergyInteractionComponent> CanergyInteraction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Canergy", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCanergyJoyScoreComponent> CanergyJoyScore;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Canergy", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCanergyBubbleRespawnComponent> CanergyBubbleRespawn;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Canergy", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCanergyCharacterProfileComponent> CanergyCharacterProfile;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Canergy", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCanergyToyWeaponControllerComponent> CanergyToyWeaponController;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Canergy", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCanergyAbilityControllerComponent> CanergyAbilityController;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Canergy", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCanergyWallTraversalComponent> CanergyWallTraversal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Canergy", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCanergyWeaponSpecialComponent> CanergyWeaponSpecial;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Canergy", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCanergyPhaseAbilityComponent> CanergyPhaseAbility;

protected:

	/** Fire weapon input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* FireAction;

	/** Switch weapon input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* SwitchWeaponAction;

	/** Name of the first person mesh weapon socket */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName FirstPersonWeaponSocket = FName("HandGrip_R");

	/** Name of the third person mesh weapon socket */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName ThirdPersonWeaponSocket = FName("HandGrip_R");

	/** Max distance to use for aim traces */
	UPROPERTY(EditAnywhere, Category ="Aim", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float MaxAimDistance = 10000.0f;

	/** Small sphere sweep used only to make relay targets forgiving to hit. */
	UPROPERTY(EditAnywhere, Category ="Aim", meta = (ClampMin = 0, ClampMax = 300, Units = "cm"))
	float RelayAimAssistRadius = 110.0f;

	/** Max HP this character can have */
	UPROPERTY(EditAnywhere, Category="Health")
	float MaxHP = 500.0f;

	/** Current HP remaining to this character */
	float CurrentHP = 0.0f;
	float BaseWalkSpeed = 600.0f;
	float BaseGravityScale = 1.0f;
	float BaseGroundFriction = 8.0f;
	float BaseBrakingDeceleration = 2048.0f;
	float BaseAirControl = 0.55f;
	float SprintMultiplier = 1.45f;
	float LiquidSpeedMultiplier = 1.6f;
	UPROPERTY(EditDefaultsOnly, Category="Canergy|Traversal", meta=(ClampMin="1.6", ClampMax="3.0"))
	float LiquidDiveSpeedMultiplier = 2.0f;
	float BaseCrouchSpeed = 300.0f;
	bool bLiquidDiveHeld = false;
	bool bLiquidDiving = false;
	// Temporary presentation adapter; disable when a visual profile supplies a crouch pose.
	UPROPERTY(EditDefaultsOnly, Category="Canergy|Presentation")
	bool bOffsetFirstPersonForPrototypeCrouch = true;
	FVector StandingFirstPersonRelativeLocation = FVector::ZeroVector;
	float FloatGravityScale = 0.28f;
	float CarnivalGravityMultiplier = 1.0f;
	float NextConductiveJoyTime = 0.0f;
	float CarryClientProbeElapsed = 0.0f;
	int32 CarryClientProbeStage = 0;
	bool bSprintHeld = false;
	CainengGameRules::FTraversalContactState TraversalContactState;
	int32 SelectedPaintMode = 0;
	int32 CurrentTraversalMode = -1;

	/** Team ID for this character*/
	UPROPERTY(EditAnywhere, Category="Team")
	uint8 TeamByte = 0;

	/** Actor tag to grant this character when it dies */
	UPROPERTY(EditAnywhere, Category="Team")
	FName DeathTag = FName("Dead");

	/** Tag to pass to weapons and projectiles to identify their AI perception noise as player-generated */
	UPROPERTY(EditAnywhere, Category="Tags")
	FName PlayerTag = FName("Player");

	/** List of weapons picked up by the character */
	TArray<AShooterWeapon*> OwnedWeapons;

	/** Weapon currently equipped and ready to shoot with */
	TObjectPtr<AShooterWeapon> CurrentWeapon;

	UPROPERTY(EditAnywhere, Category ="Destruction", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float RespawnTime = 5.0f;

	FTimerHandle RespawnTimer;

public:

	/** Bullet count updated delegate */
	FBulletCountUpdatedDelegate OnBulletCountUpdated;

	/** Damaged delegate */
	FDamagedDelegate OnDamaged;

public:

	/** Constructor */
	AShooterCharacter();

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Gameplay cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	void UpdateTraversalSurface(float DeltaSeconds);
	void PaintSurfaceAtAim();
	void CyclePaintMode();
	void CycleSelectedAbility();
	void ActivateSelectedAbility();
	void ActivateWeaponSpecial();
	void ActivatePhaseAbility();
	void RequestNextRound();
	void InteractCarryCore();
	UFUNCTION(Server, Reliable) void ServerInteractCarryCore();
	void StartSprint();
	void StopSprint();
	void StartLiquidDive();
	void StopLiquidDive();
	void UpdateLiquidDive();
	void UpdateMovementTuning();
	void ShowPrototypeFeedback(const FText& Message, float Duration = 1.8f);
	UFUNCTION()
	void HandleCanergyAbilityFeedback(FText Message);
	UFUNCTION()
	void HandleCanergyWeaponFeedback(FText Message);
	UFUNCTION()
	void HandleBubbleStateChanged();

public:

	/** Handle incoming damage */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:

	/** Handles aim inputs from either controls or UI interfaces */
	virtual void DoAim(float Yaw, float Pitch) override;

	/** Handles move inputs from either controls or UI interfaces */
	virtual void DoMove(float Right, float Forward)  override;

	/** Handles jump start inputs from either controls or UI interfaces */
	virtual void DoJumpStart()  override;

	/** Handles jump end inputs from either controls or UI interfaces */
	virtual void DoJumpEnd()  override;

	/** Handles start firing input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStartFiring();

	/** Handles stop firing input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStopFiring();

	/** Handles switch weapon input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSwitchWeapon();
	void SetCarnivalGravityMultiplier(float NewMultiplier);
	/** Shared input command for player controls and development/Bot drivers. */
	UFUNCTION(BlueprintCallable, Category="Canergy|Traversal")
	void SetLiquidDiveHeld(bool bHeld);
	bool IsLiquidDiving() const { return bLiquidDiving; }
	bool IsWallTraversing() const;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

public:

	//~Begin IShooterWeaponHolder interface

	/** Attaches a weapon's meshes to the owner */
	virtual void AttachWeaponMeshes(AShooterWeapon* Weapon) override;

	/** Plays the firing montage for the weapon */
	virtual void PlayFiringMontage(UAnimMontage* Montage) override;

	/** Applies weapon recoil to the owner */
	virtual void AddWeaponRecoil(float Recoil) override;

	/** Updates the weapon's HUD with the current ammo count */
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) override;

	/** Calculates and returns the aim location for the weapon */
	virtual FVector GetWeaponTargetLocation() override;

	/** Gives a weapon of this class to the owner */
	virtual void AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass) override;

	/** Activates the passed weapon */
	virtual void OnWeaponActivated(AShooterWeapon* Weapon) override;

	/** Deactivates the passed weapon */
	virtual void OnWeaponDeactivated(AShooterWeapon* Weapon) override;

	/** Notifies the owner that the weapon cooldown has expired and it's ready to shoot again */
	virtual void OnSemiWeaponRefire() override;

	//~End IShooterWeaponHolder interface

protected:

	/** Returns true if the character already owns a weapon of the given class */
	AShooterWeapon* FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const;

	/** Called when this character's HP is depleted */
	void Die();

	/** Called to allow Blueprint code to react to this character's death */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "On Death"))
	void BP_OnDeath();

	/** Called from the respawn timer to destroy this character and force the PC to respawn */
	void OnRespawn();

public:

	/** Returns true if the character is dead */
	bool IsDead() const;

	UCanergyInteractionComponent* GetCanergyInteraction() const { return CanergyInteraction; }
	UCanergyJoyScoreComponent* GetCanergyJoyScore() const { return CanergyJoyScore; }
	UCanergyBubbleRespawnComponent* GetCanergyBubbleRespawn() const { return CanergyBubbleRespawn; }
	UCanergyCharacterProfileComponent* GetCanergyCharacterProfile() const { return CanergyCharacterProfile; }
	UCanergyAbilityControllerComponent* GetCanergyAbilityController() const { return CanergyAbilityController; }

	/** Sets the team ID for this character */
	void SetTeam(uint8 Team);
};
