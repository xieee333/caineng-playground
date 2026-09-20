// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTypes.h"

namespace CainengGameRules
{
	inline FCanergyMatchRuntimeState StartMatch(const FCanergyMatchRulesSettings& Settings)
	{
		FCanergyMatchRuntimeState State;
		State.Phase = ECanergyMatchPhase::FreePlay;
		State.PhaseRemainingSeconds = FMath::Max(0.0f, Settings.InitialFreePlaySeconds);
		if (Settings.RoundDurationSeconds <= 0.0f)
		{
			State.Phase = ECanergyMatchPhase::Finished;
			State.PhaseRemainingSeconds = 0.0f;
		}
		return State;
	}

	inline bool TryStartPublicObjective(FCanergyMatchRuntimeState& State,
		ECanergyPublicObjectiveKind Objective, float DurationSeconds)
	{
		if (State.Phase != ECanergyMatchPhase::PublicObjective
			|| State.bPublicObjectiveResolved
			|| State.ActiveObjective != ECanergyPublicObjectiveKind::None
			|| Objective == ECanergyPublicObjectiveKind::None
			|| DurationSeconds <= 0.0f)
		{
			return false;
		}

		State.ActiveObjective = Objective;
		State.ActiveObjectiveRemainingSeconds = DurationSeconds;
		return true;
	}

	inline bool CompletePublicObjective(FCanergyMatchRuntimeState& State)
	{
		if (State.Phase != ECanergyMatchPhase::PublicObjective
			|| State.ActiveObjective == ECanergyPublicObjectiveKind::None)
		{
			return false;
		}

		State.ActiveObjective = ECanergyPublicObjectiveKind::None;
		State.ActiveObjectiveRemainingSeconds = 0.0f;
		State.bPublicObjectiveResolved = true;
		return true;
	}

	inline float AddCarnivalMeter(FCanergyMatchRuntimeState& State, float Amount,
		const FCanergyMatchRulesSettings& Settings)
	{
		if (State.Phase == ECanergyMatchPhase::Waiting || State.Phase == ECanergyMatchPhase::Finished)
		{
			return State.CarnivalMeter;
		}
		const float Maximum = FMath::Max(0.0f, Settings.CarnivalMeterThreshold);
		State.CarnivalMeter = FMath::Clamp(State.CarnivalMeter + FMath::Max(0.0f, Amount), 0.0f, Maximum);
		return State.CarnivalMeter;
	}

	inline bool CanStartCarnivalEvent(const FCanergyMatchRuntimeState& State,
		const FCanergyMatchRulesSettings& Settings)
	{
		return State.Phase != ECanergyMatchPhase::Waiting
			&& State.Phase != ECanergyMatchPhase::Finished
			&& !State.IsCarnivalActive()
			&& Settings.CarnivalMeterThreshold > 0.0f
			&& State.CarnivalMeter >= Settings.CarnivalMeterThreshold;
	}

	inline bool TryStartCarnivalEvent(FCanergyMatchRuntimeState& State,
		ECanergyCarnivalEventKind Event, const FCanergyMatchRulesSettings& Settings)
	{
		if (!CanStartCarnivalEvent(State, Settings) || Event == ECanergyCarnivalEventKind::None)
		{
			return false;
		}

		State.ActiveCarnivalEvent = Event;
		State.CarnivalRemainingSeconds = FMath::Clamp(Settings.CarnivalEventDurationSeconds, 5.0f, 30.0f);
		State.CarnivalMeter = 0.0f;
		return true;
	}

	inline void FinishMatch(FCanergyMatchRuntimeState& State)
	{
		State.Phase = ECanergyMatchPhase::Finished;
		State.PhaseRemainingSeconds = 0.0f;
		State.ActiveObjective = ECanergyPublicObjectiveKind::None;
		State.ActiveObjectiveRemainingSeconds = 0.0f;
		State.bPublicObjectiveResolved = true;
		State.ActiveCarnivalEvent = ECanergyCarnivalEventKind::None;
		State.CarnivalRemainingSeconds = 0.0f;
	}

	/** Advances phase clocks without skipping transitions when a frame delta is unusually large. */
	inline FCanergyMatchRuntimeState AdvanceMatchClock(FCanergyMatchRuntimeState State,
		float DeltaSeconds, const FCanergyMatchRulesSettings& Settings)
	{
		if (State.Phase == ECanergyMatchPhase::Waiting || State.Phase == ECanergyMatchPhase::Finished)
		{
			return State;
		}

		float RemainingDelta = FMath::Max(0.0f, DeltaSeconds);
		const float RoundDuration = FMath::Max(0.0f, Settings.RoundDurationSeconds);
		int32 TransitionGuard = 0;

		while (++TransitionGuard <= 8)
		{
			if (State.ElapsedSeconds >= RoundDuration)
			{
				FinishMatch(State);
				break;
			}

			if (State.PhaseRemainingSeconds <= KINDA_SMALL_NUMBER)
			{
				if (State.Phase == ECanergyMatchPhase::FreePlay)
				{
					State.Phase = ECanergyMatchPhase::PublicObjective;
					State.PhaseRemainingSeconds = FMath::Max(0.05f, Settings.PublicObjectivePhaseSeconds);
					State.bPublicObjectiveResolved = false;
					State.ActiveObjective = ECanergyPublicObjectiveKind::None;
					State.ActiveObjectiveRemainingSeconds = 0.0f;
					continue;
				}
				if (State.Phase == ECanergyMatchPhase::PublicObjective)
				{
					State.ActiveObjective = ECanergyPublicObjectiveKind::None;
					State.ActiveObjectiveRemainingSeconds = 0.0f;
					State.bPublicObjectiveResolved = true;
					State.Phase = ECanergyMatchPhase::FreePlay;
					State.PhaseRemainingSeconds = FMath::Max(0.0f, Settings.RepeatFreePlaySeconds);
					continue;
				}
				break;
			}

			if (RemainingDelta <= KINDA_SMALL_NUMBER)
			{
				break;
			}

			const float UntilRoundEnd = RoundDuration - State.ElapsedSeconds;
			const float Step = FMath::Min3(RemainingDelta, State.PhaseRemainingSeconds, UntilRoundEnd);
			if (Step <= KINDA_SMALL_NUMBER)
			{
				FinishMatch(State);
				break;
			}

			State.ElapsedSeconds += Step;
			State.PhaseRemainingSeconds -= Step;
			RemainingDelta -= Step;
			if (State.ActiveObjective != ECanergyPublicObjectiveKind::None)
			{
				State.ActiveObjectiveRemainingSeconds = FMath::Max(0.0f,
					State.ActiveObjectiveRemainingSeconds - Step);
				if (State.ActiveObjectiveRemainingSeconds <= KINDA_SMALL_NUMBER)
				{
					State.ActiveObjective = ECanergyPublicObjectiveKind::None;
					State.bPublicObjectiveResolved = true;
				}
			}

			if (State.ElapsedSeconds >= RoundDuration)
			{
				FinishMatch(State);
				break;
			}
		}

		if (State.IsCarnivalActive())
		{
			State.CarnivalRemainingSeconds = FMath::Max(0.0f,
				State.CarnivalRemainingSeconds - FMath::Max(0.0f, DeltaSeconds));
			if (State.CarnivalRemainingSeconds <= KINDA_SMALL_NUMBER)
			{
				State.ActiveCarnivalEvent = ECanergyCarnivalEventKind::None;
				State.CarnivalRemainingSeconds = 0.0f;
			}
		}
		return State;
	}
}
