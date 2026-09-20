#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CarryObjectiveRules.h"
#include "CarryPrototype.generated.h"

class AShooterCharacter;
class AStaticMeshActor;
class UTextRenderComponent;
class ACanergyBounceFlower;

/** Opt-in local slice. Networking and final visuals are deliberately separate milestones. */
UCLASS()
class TP_FIRSTPERSON_API ACanergyCarryPrototype : public AActor
{
	GENERATED_BODY()
public:
	ACanergyCarryPrototype();
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void Interact(AShooterCharacter* Character);
	void Dislodge(AShooterCharacter* Character);
	void HandleWeaponHit(AShooterCharacter* Target, AShooterCharacter* Attacker);
	bool IsFinished() const { return Rules.bFinished; }
	bool IsCarriedBy(const AShooterCharacter* Character) const { return FindPlayer(Character) == Rules.Carrier; }
private:
	CainengGameRules::FCarryObjectiveState Rules;
	UPROPERTY() TObjectPtr<AStaticMeshActor> Core;
	TArray<TWeakObjectPtr<AShooterCharacter>> Players;
	TArray<float> ProtectedUntil;
	TArray<bool> PreviousBubbled;
	TArray<bool> BotFlankReached;
	UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> TeamLabels;
	FVector Arena = FVector::ZeroVector;
	FVector Goals[2];
	static constexpr float ArenaHalfLength = 2700.0f;
	static constexpr float ArenaHalfWidth = 2000.0f;
	static constexpr float GoalOffset = 2250.0f;
	static constexpr float SpawnOffset = 1900.0f;
	float Age = 0.0f, HudClock = 0.0f, LooseClock = 0.0f, InitializeAfter = 2.0f;
	bool bReady = false, bProbe = false, bPractice = false, bNetworkProbe = false, bBotMatch = false, bProbePass = true, bReported = false;
	bool bObjectiveToolProbe = false;
	bool bObjectiveToolProbePass = true;
	int32 ProbeStep = 0;
	int32 ObjectiveToolProbeStep = 0;
	float ObjectiveToolProbeInitialDistance = 0.0f;
	float NextProbe = 0.0f;
	TWeakObjectPtr<ACanergyBounceFlower> ObjectiveToolProbeFlower;
	UPROPERTY(ReplicatedUsing=OnRep_CarrySnapshot) int32 RepScoreA = 0;
	UPROPERTY(ReplicatedUsing=OnRep_CarrySnapshot) int32 RepScoreB = 0;
	UPROPERTY(ReplicatedUsing=OnRep_CarrySnapshot) int32 RepCarrierTeam = INDEX_NONE;
	UPROPERTY(ReplicatedUsing=OnRep_CarrySnapshot) float RepRemaining = 180.0f;
	UPROPERTY(ReplicatedUsing=OnRep_CarrySnapshot) bool bRepFinished = false;
	UPROPERTY(ReplicatedUsing=OnRep_CarrySnapshot) TObjectPtr<AActor> RepCarrier;
	UFUNCTION() void OnRep_CarrySnapshot();
	AStaticMeshActor* MakeShape(const TCHAR* Asset, FVector Position, FVector Scale, FLinearColor Color);
	void InitializeArena();
	void ResetCore();
	void UpdateHud();
	void DriveBots(float DeltaSeconds);
	void RunProbe();
	void RunObjectiveToolProbe();
	int32 FindPlayer(const AShooterCharacter* Character) const;
	bool CanPlayerContest(int32 Index) const;
	void SyncReplicatedSnapshot();
	int32 Team(int32 Index) const { return Index % 2; }
};
