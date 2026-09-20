#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TraversalProbe.generated.h"

class AShooterCharacter;
class AStaticMeshActor;
class UCanergyPaintableSurfaceComponent;
class ACanergyBounceFlower;

/** Opt-in visible development probe. Never spawned by normal gameplay. */
UCLASS(NotBlueprintable, Transient)
class ACanergyTraversalProbe : public AActor
{
	GENERATED_BODY()
public:
	ACanergyTraversalProbe();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	TWeakObjectPtr<AShooterCharacter> Character;
	TWeakObjectPtr<AStaticMeshActor> Floor;
	TWeakObjectPtr<AStaticMeshActor> Ceiling;
	TWeakObjectPtr<AStaticMeshActor> Wall;
	TWeakObjectPtr<UCanergyPaintableSurfaceComponent> WallSurface;
	TWeakObjectPtr<AStaticMeshActor> BloomProp;
	TWeakObjectPtr<ACanergyBounceFlower> TestFlower;
	TWeakObjectPtr<UCanergyPaintableSurfaceComponent> Surface;
	FTransform OriginalTransform;
	FRotator OriginalView;
	FVector Origin = FVector::ZeroVector;
	FVector PhaseStart = FVector::ZeroVector;
	float Time = 0.0f;
	float NormalDistance = 0.0f;
	float LiquidDistance = 0.0f;
	float BaseGravity = 1.0f;
	float MaximumUpSpeed = 0.0f;
	int32 Phase = -1;
	bool bJumped = false;
	bool bFloatAwayFromGround = false;
	bool bGravityRestored = false;
	bool bBubbleRulesOk = false;
	float BubbleReturnTime = 0.0f;
	int32 JoyBeforeBubble = 0;
	float StandingHalfHeight = 0.0f;
	float DivePeakSpeed = 0.0f;
	bool bDiveEntered = false;
	bool bDiveReleased = false;
	bool bDiveReentered = false;
	bool bDiveSurfaceExit = false;
	float StandingCameraHeight = 0.0f;
	float DiveCameraDrop = 0.0f;
	bool bCeilingCrouchSafe = false;
	bool bCeilingExitRestored = false;
	bool bWallEntered[4] = {};
	float WallHeights[4] = {};
	bool bWallReleaseFalling = false;
	bool bWallSurfaceLossFalling = false;
	bool bWallJumpAway = false;
	bool bWallBubbleStarted = false;
	bool bWallBubbleCancelled = false;
	bool bWallBubbleReturned = false;
	bool bBloomAttempted = false;
	bool bBloomSpawned = false;
	bool bBloomCooldownProtected = false;
	bool bBloomBubbleProtected = false;
	bool bBloomExpired = false;
	float BloomCharacterUpSpeed = 0.0f;
	float BloomPropUpSpeed = 0.0f;
	void StartPhase(int32 NewPhase);
	void Finish();
};
