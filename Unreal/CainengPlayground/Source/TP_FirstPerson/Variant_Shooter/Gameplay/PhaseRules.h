#pragma once
#include "CoreMinimal.h"

namespace CainengGameRules
{
	struct FPhaseState
	{
		float ActiveRemaining = 0.0f;
		float CooldownRemaining = 0.0f;
	};

	inline bool CanBeginPhase(const FPhaseState& State, bool bDisabled, bool bTaggedWall,
		float WallThickness, float MaxThickness, bool bExitClear)
	{
		return !bDisabled && State.ActiveRemaining <= 0.0f && State.CooldownRemaining <= 0.0f
			&& bTaggedWall && FMath::IsFinite(WallThickness) && WallThickness > 0.0f
			&& WallThickness <= MaxThickness && bExitClear;
	}

	inline bool BeginPhase(FPhaseState& State, bool bDisabled, bool bTaggedWall,
		float WallThickness, float MaxThickness, bool bExitClear, float Duration = 1.5f, float Cooldown = 8.0f)
	{
		if (!CanBeginPhase(State, bDisabled, bTaggedWall, WallThickness, MaxThickness, bExitClear)) return false;
		State.ActiveRemaining = FMath::Max(0.1f, Duration);
		State.CooldownRemaining = FMath::Max(State.ActiveRemaining, Cooldown);
		return true;
	}

	inline void AdvancePhase(FPhaseState& State, float DeltaSeconds)
	{
		if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) return;
		State.ActiveRemaining = FMath::Max(0.0f, State.ActiveRemaining - DeltaSeconds);
		State.CooldownRemaining = FMath::Max(0.0f, State.CooldownRemaining - DeltaSeconds);
	}

	inline bool BreakPhase(FPhaseState& State)
	{
		if (State.ActiveRemaining <= 0.0f) return false;
		State.ActiveRemaining = 0.0f;
		return true;
	}
}
