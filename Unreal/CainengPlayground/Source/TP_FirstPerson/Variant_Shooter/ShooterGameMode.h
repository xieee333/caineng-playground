// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Variant_Shooter/Gameplay/GameplayTypes.h"
#include "ShooterGameMode.generated.h"

class UShooterUI;
class URelayObjectiveWidget;
class AStaticMeshActor;
class ACanergyMatchGameState;
class ACanergyCarryPrototype;

/**
 *  Simple GameMode for a first person shooter game
 *  Manages game UI
 *  Keeps track of team scores
 */
UCLASS(abstract)
class TP_FIRSTPERSON_API AShooterGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
protected:

	/** Type of UI widget to spawn */
	UPROPERTY(EditAnywhere, Category="Shooter")
	TSubclassOf<UShooterUI> ShooterUIClass;

	/** Pointer to the UI widget */
	TObjectPtr<UShooterUI> ShooterUI;
	TObjectPtr<URelayObjectiveWidget> RelayObjectiveWidget;
	UPROPERTY() TObjectPtr<ACanergyCarryPrototype> CarryPrototype;

	/** Map of scores by team ID */
	TMap<uint8, int32> TeamScores;

protected:

	/** Determines how many local players should be spawned on game start */
	UPROPERTY(EditDefaultsOnly, Category="Local Multiplayer", meta = (ClampMin = 1, ClampMax = 4))
	int32 NumberOfLocalPlayers = 1;

	/** Optional combat bots; disabled by default so the traversal prototype can be tested without interruption. */
	UPROPERTY(EditDefaultsOnly, Category="Shooter|AI")
	bool bSpawnEnemyNPCs = false;

	/** Used to assign players to different PlayerStarts in the level */
	int32 CurrentPlayerStartAssignment = 0;
	int32 RelayProgress = 0;
	AStaticMeshActor* RelayGate = nullptr;
	TArray<AStaticMeshActor*> RelayTargets;
	FVector RelayExitLocation = FVector::ZeroVector;
	FVector RelayHurdleLocation = FVector::ZeroVector;
	FVector RelayForwardDirection = FVector::ForwardVector;
	FVector RelayRightDirection = FVector::RightVector;
	float PreviousRelayHurdleSide = 0.0f;
	bool bReachedExit = false;
	bool bJumpedCalibrationHurdle = false;

	UPROPERTY(EditDefaultsOnly, Category="Canergy|Match")
	FCanergyMatchRulesSettings MatchRulesSettings;

	FCanergyMatchRuntimeState MatchRuntimeState;
	FVector MatchAnchorLocation = FVector::ZeroVector;
	FVector MatchForwardDirection = FVector::ForwardVector;
	FVector MatchRightDirection = FVector::RightVector;
	TArray<TWeakObjectPtr<AStaticMeshActor>> PublicObjectiveActors;
	TSet<TWeakObjectPtr<AActor>> PublicObjectivePaintedActors;
	TSet<TWeakObjectPtr<AActor>> PublicObjectiveContributors;
	TArray<TWeakObjectPtr<AStaticMeshActor>> CarnivalCollectibles;
	int32 CarnivalEventCursor = 0;
	bool bObjectiveSpawnedThisPhase = false;
	bool bCarnivalEventWasActive = false;

	void SpawnColorTraversalTrack(const AActor* PlayerStart);
	void SpawnRelayObjective(const AActor* PlayerStart);
	void SpawnPublicObjective();
	void ClearPublicObjective();
	void SpawnCarnivalBalloons();
	void ApplyCarnivalEvent(ECanergyCarnivalEventKind EventKind);
	void UpdateReplicatedMatchState();
	void UpdateRelayObjectiveWidget();

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Assigns a PlayerStart to a specific player */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

public:
	AShooterGameMode();
	void RegisterRelayHit(AActor* HitActor, int32 PaintMode);
	void RegisterPublicObjectiveHit(AActor* Target, AActor* Contributor);
	void AddCarnivalMeter(float Amount);
	const FCanergyMatchRuntimeState& GetMatchRuntimeState() const { return MatchRuntimeState; }
	void ShowPlayerFeedback(const FText& Feedback, float Duration = 1.8f);
	void RequestNewRound();
	void SetPrototypeStatus(const FText& Status);
	ACanergyCarryPrototype* GetCarryPrototype() const { return CarryPrototype; }

	/** Increases the score for the given team */
	void IncrementTeamScore(uint8 TeamByte);

	/** Returns true if enemy NPCs should be used */
	bool ShouldSpawnEnemyNPCs() const;
};
