#pragma once

#include "CoreMinimal.h"

namespace CainengGameRules
{
	inline bool IsClimbableLiquidWall(bool bLiquid, float NormalZ)
	{
		return bLiquid && FMath::IsFinite(NormalZ) && FMath::Abs(NormalZ) <= 0.2f;
	}

	inline bool CanLiquidDive(bool bHeld, bool bGrounded, bool bLiquidSurface, bool bDisabled)
	{
		return bHeld && bGrounded && bLiquidSurface && !bDisabled;
	}

	struct FTraversalContactState
	{
		bool bBounceGroundContact = false;
		float FloatSecondsRemaining = 0.0f;
	};

	// A nearby surface is not a landing. Only grounded contact arms movement effects.
	inline bool AdvanceTraversalContact(FTraversalContactState& State, bool bGrounded,
		bool bBounceSurface, bool bFloatSurface, float DeltaSeconds, bool bDisabled = false)
	{
		if (bDisabled)
		{
			State = {};
			return false;
		}
		const bool bBounceContact = bGrounded && bBounceSurface;
		const bool bLaunch = bBounceContact && !State.bBounceGroundContact;
		State.bBounceGroundContact = bBounceContact;
		if (bGrounded)
		{
			State.FloatSecondsRemaining = bFloatSurface ? 1.5f : 0.0f;
		}
		else
		{
			State.FloatSecondsRemaining = FMath::Max(0.0f,
				State.FloatSecondsRemaining - FMath::Max(0.0f, DeltaSeconds));
		}
		return bLaunch;
	}
}
