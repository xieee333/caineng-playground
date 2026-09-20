// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTypes.h"

namespace CainengGameRules
{
	inline float ClampEffectDuration(float DurationSeconds, float MaxDurationSeconds = 3.0f)
	{
		const float SafeMaximum = FMath::Max(0.05f, MaxDurationSeconds);
		return FMath::Clamp(DurationSeconds, 0.0f, SafeMaximum);
	}

	/** Applies a timed effect deterministically; effects never deal damage by themselves. */
	inline FCanergyActiveInteractionEffect ApplyInteractionEffect(
		const FCanergyActiveInteractionEffect& Current,
		const FCanergyInteractionEffectSpec& Incoming,
		float MaxDurationSeconds = 3.0f)
	{
		const float IncomingDuration = ClampEffectDuration(Incoming.DurationSeconds, MaxDurationSeconds);
		if (Incoming.Kind == ECanergyInteractionKind::None || IncomingDuration <= 0.0f)
		{
			return Current;
		}

		const uint8 IncomingMaxStacks = FMath::Max<uint8>(1, Incoming.MaxStacks);
		const FCanergyActiveInteractionEffect NewEffect{
			Incoming.Kind,
			IncomingDuration,
			FMath::Max(0.0f, Incoming.Magnitude),
			1
		};

		if (!Current.IsActive() || Current.Kind != Incoming.Kind
			|| Incoming.StackPolicy == ECanergyEffectStackPolicy::Replace)
		{
			return NewEffect;
		}

		FCanergyActiveInteractionEffect Result = Current;
		Result.Magnitude = FMath::Max(Result.Magnitude, NewEffect.Magnitude);
		switch (Incoming.StackPolicy)
		{
		case ECanergyEffectStackPolicy::RefreshDuration:
			Result.RemainingSeconds = IncomingDuration;
			break;
		case ECanergyEffectStackPolicy::KeepLongest:
			Result.RemainingSeconds = FMath::Max(Result.RemainingSeconds, IncomingDuration);
			break;
		case ECanergyEffectStackPolicy::AddStacks:
			Result.RemainingSeconds = FMath::Max(Result.RemainingSeconds, IncomingDuration);
			Result.StackCount = static_cast<uint8>(FMath::Min<int32>(
				static_cast<int32>(Result.StackCount) + 1,
				static_cast<int32>(IncomingMaxStacks)));
			break;
		default:
			return NewEffect;
		}
		return Result;
	}

	inline FCanergyActiveInteractionEffect TickInteractionEffect(
		const FCanergyActiveInteractionEffect& Current, float DeltaSeconds)
	{
		if (!Current.IsActive())
		{
			return {};
		}

		FCanergyActiveInteractionEffect Result = Current;
		Result.RemainingSeconds = FMath::Max(0.0f, Result.RemainingSeconds - FMath::Max(0.0f, DeltaSeconds));
		return Result.RemainingSeconds > 0.0f ? Result : FCanergyActiveInteractionEffect{};
	}

	/** Keeps independent interaction kinds active together while applying stacking per kind. */
	inline TArray<FCanergyActiveInteractionEffect> ApplyInteractionEffect(
		const TArray<FCanergyActiveInteractionEffect>& CurrentEffects,
		const FCanergyInteractionEffectSpec& Incoming,
		float MaxDurationSeconds = 3.0f)
	{
		if (Incoming.Kind == ECanergyInteractionKind::None || Incoming.DurationSeconds <= 0.0f)
		{
			return CurrentEffects;
		}

		TArray<FCanergyActiveInteractionEffect> Result = CurrentEffects;
		for (FCanergyActiveInteractionEffect& Existing : Result)
		{
			if (Existing.Kind == Incoming.Kind)
			{
				Existing = ApplyInteractionEffect(Existing, Incoming, MaxDurationSeconds);
				return Result;
			}
		}

		Result.Add(ApplyInteractionEffect(FCanergyActiveInteractionEffect{}, Incoming, MaxDurationSeconds));
		return Result;
	}

	inline TArray<FCanergyActiveInteractionEffect> TickInteractionEffects(
		const TArray<FCanergyActiveInteractionEffect>& CurrentEffects, float DeltaSeconds)
	{
		TArray<FCanergyActiveInteractionEffect> Result;
		Result.Reserve(CurrentEffects.Num());
		for (const FCanergyActiveInteractionEffect& Effect : CurrentEffects)
		{
			FCanergyActiveInteractionEffect Ticked = TickInteractionEffect(Effect, DeltaSeconds);
			if (Ticked.IsActive())
			{
				Result.Add(Ticked);
			}
		}
		return Result;
	}

	inline constexpr float BubbleRespawnDelaySeconds = 3.0f;

	/** Bubble recovery is a short return-to-play timer, not a permanent-resource penalty. */
	inline bool CanRespawnFromBubble(float TimeInBubbleSeconds, float RequiredDelaySeconds = BubbleRespawnDelaySeconds)
	{
		return TimeInBubbleSeconds >= FMath::Max(0.0f, RequiredDelaySeconds);
	}

	inline int32 GetJoyAward(ECanergyJoyAction Action)
	{
		switch (Action)
		{
		case ECanergyJoyAction::SurfaceTrail: return 1;
		case ECanergyJoyAction::BounceChain: return 3;
		case ECanergyJoyAction::Rescue: return 5;
		case ECanergyJoyAction::PublicObjective: return 5;
		case ECanergyJoyAction::TrickShot: return 2;
		case ECanergyJoyAction::CarnivalParticipation: return 3;
		case ECanergyJoyAction::RouteDiscovery: return 2;
		case ECanergyJoyAction::EnvironmentalChain: return 4;
		case ECanergyJoyAction::Collectible: return 1;
		default: return 0;
		}
	}
}
