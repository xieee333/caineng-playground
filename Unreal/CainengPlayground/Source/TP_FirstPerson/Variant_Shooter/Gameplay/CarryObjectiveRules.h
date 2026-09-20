#pragma once
#include "CoreMinimal.h"

namespace CainengGameRules
{
	/** World-independent ownership: one carrier, one scoring transition, bounded round. */
	struct FCarryObjectiveState
	{
		int32 Carrier = INDEX_NONE;
		int32 CarrierTeam = INDEX_NONE;
		int32 Scores[2] = {0, 0};
		float Remaining = 180.0f;
		float PickupLock = 0.0f;
		bool bFinished = false;
	};
	inline bool TryTakeCore(FCarryObjectiveState& State, int32 Player, int32 Team, bool bEligible, bool bInReach)
	{
		if (State.bFinished || State.Carrier != INDEX_NONE || State.PickupLock > 0.0f
			|| Player < 0 || Team < 0 || Team > 1 || !bEligible || !bInReach) return false;
		State.Carrier = Player;
		State.CarrierTeam = Team;
		return true;
	}
	inline bool CanContestCore(bool bDead, bool bBubbled, float SpawnProtectionSeconds)
	{
		return !bDead && !bBubbled && FMath::IsFinite(SpawnProtectionSeconds) && SpawnProtectionSeconds <= 0.0f;
	}
	inline bool DropCore(FCarryObjectiveState& State, int32 Player)
	{
		if (State.bFinished || Player < 0 || State.Carrier != Player) return false;
		State.Carrier = State.CarrierTeam = INDEX_NONE;
		State.PickupLock = 0.6f;
		return true;
	}
	inline bool TryDeliverCore(FCarryObjectiveState& State, int32 Player, int32 GoalTeam, bool bInGoal)
	{
		if (State.bFinished || Player < 0 || State.Carrier != Player || !bInGoal
			|| GoalTeam < 0 || GoalTeam > 1 || State.CarrierTeam != GoalTeam) return false;
		++State.Scores[GoalTeam];
		State.Carrier = State.CarrierTeam = INDEX_NONE;
		State.PickupLock = 2.0f;
		State.bFinished = State.Scores[GoalTeam] >= 3;
		return true;
	}
	inline void AdvanceCarryClock(FCarryObjectiveState& State, float DeltaSeconds)
	{
		if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f || State.bFinished) return;
		State.PickupLock = FMath::Max(0.0f, State.PickupLock - DeltaSeconds);
		State.Remaining = FMath::Max(0.0f, State.Remaining - DeltaSeconds);
		State.bFinished = State.Remaining <= 0.0f;
	}
}
