// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

enum class ERelayHitResolution : uint8
{
	Invalid,
	Advanced,
	AlreadyCalibrated,
	WrongOrder
};

struct FRelayHitOutcome
{
	ERelayHitResolution Resolution = ERelayHitResolution::Invalid;
	int32 NewProgress = 0;
};

namespace CainengGameRules
{
	/** Resolves the cyan -> orange -> magenta relay sequence without world/UI dependencies. */
	inline FRelayHitOutcome ResolveRelayHit(int32 CurrentProgress, int32 HitIndex)
	{
		CurrentProgress = FMath::Clamp(CurrentProgress, 0, 3);
		if (HitIndex < 0 || HitIndex > 2 || CurrentProgress >= 3)
		{
			return { ERelayHitResolution::Invalid, CurrentProgress };
		}

		if (HitIndex < CurrentProgress)
		{
			return { ERelayHitResolution::AlreadyCalibrated, CurrentProgress };
		}

		if (HitIndex == CurrentProgress)
		{
			return { ERelayHitResolution::Advanced, CurrentProgress + 1 };
		}

		// Preserve partial recovery: accidentally hitting cyan while out of order
		// leaves the first relay solved; any later color restarts the sequence.
		return { ERelayHitResolution::WrongOrder, HitIndex == 0 ? 1 : 0 };
	}

	inline bool CrossedCalibrationHurdle(float PreviousSide, float CurrentSide, float LateralDistance,
		bool bIsFalling, float MaxLateralDistance = 185.0f)
	{
		return PreviousSide <= 0.0f && CurrentSide > 0.0f && bIsFalling
			&& FMath::Abs(LateralDistance) <= MaxLateralDistance;
	}

	inline bool CanCompleteRelayTrial(int32 RelayProgress, bool bJumpedCalibrationHurdle,
		float ExitDistanceSquared, float ExitRadius = 240.0f)
	{
		return RelayProgress >= 3 && bJumpedCalibrationHurdle
			&& ExitDistanceSquared <= FMath::Square(FMath::Max(0.0f, ExitRadius));
	}
}
