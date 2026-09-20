// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Variant_Shooter/Gameplay/MatchLoopRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanergyMatchLoopRulesTest,
	"Caineng.Gameplay.Core.MatchLoop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanergyMatchLoopRulesTest::RunTest(const FString& Parameters)
{
	FCanergyMatchRulesSettings Settings;
	FCanergyMatchRuntimeState State = CainengGameRules::StartMatch(Settings);
	TestEqual(TEXT("A new round begins in free play"), static_cast<uint8>(State.Phase),
		static_cast<uint8>(ECanergyMatchPhase::FreePlay));
	TestTrue(TEXT("The first free-play window lasts sixty seconds"), FMath::IsNearlyEqual(State.PhaseRemainingSeconds, 60.0f));

	State = CainengGameRules::AdvanceMatchClock(State, 59.9f, Settings);
	TestEqual(TEXT("The optional objective does not start early"), static_cast<uint8>(State.Phase),
		static_cast<uint8>(ECanergyMatchPhase::FreePlay));
	State = CainengGameRules::AdvanceMatchClock(State, 0.1f, Settings);
	TestEqual(TEXT("The objective phase starts at the free-play boundary"), static_cast<uint8>(State.Phase),
		static_cast<uint8>(ECanergyMatchPhase::PublicObjective));
	TestTrue(TEXT("The objective window is three and a half minutes"), FMath::IsNearlyEqual(State.PhaseRemainingSeconds, 210.0f));

	TestFalse(TEXT("An unspecified task cannot be started"), CainengGameRules::TryStartPublicObjective(
		State, ECanergyPublicObjectiveKind::None, 60.0f));
	TestTrue(TEXT("A public task can be offered during its optional phase"), CainengGameRules::TryStartPublicObjective(
		State, ECanergyPublicObjectiveKind::BalloonChase, 90.0f));
	TestFalse(TEXT("A second task cannot overwrite the active one"), CainengGameRules::TryStartPublicObjective(
		State, ECanergyPublicObjectiveKind::FeedCreature, 90.0f));
	TestTrue(TEXT("Completing a public task is accepted"), CainengGameRules::CompletePublicObjective(State));
	TestTrue(TEXT("Task completion does not end or punish the match"), State.Phase == ECanergyMatchPhase::PublicObjective
		&& State.bPublicObjectiveResolved && State.ElapsedSeconds < Settings.RoundDurationSeconds);
	TestFalse(TEXT("The same objective phase cannot be farmed by restarting tasks"), CainengGameRules::TryStartPublicObjective(
		State, ECanergyPublicObjectiveKind::FeedCreature, 90.0f));

	State = CainengGameRules::AdvanceMatchClock(State, 210.0f, Settings);
	TestEqual(TEXT("The loop returns to free play after the objective window"), static_cast<uint8>(State.Phase),
		static_cast<uint8>(ECanergyMatchPhase::FreePlay));
	TestTrue(TEXT("A short free-play intermission precedes the next cycle"), FMath::IsNearlyEqual(State.PhaseRemainingSeconds, 45.0f));
	State = CainengGameRules::AdvanceMatchClock(State, 150.0f, Settings);
	TestTrue(TEXT("Large frame deltas do not skip the next phase or round end"), State.Phase == ECanergyMatchPhase::Finished
		&& FMath::IsNearlyEqual(State.ElapsedSeconds, 420.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanergyObjectiveTimeoutRulesTest,
	"Caineng.Gameplay.Core.ObjectiveTimeout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanergyObjectiveTimeoutRulesTest::RunTest(const FString& Parameters)
{
	FCanergyMatchRulesSettings Settings;
	FCanergyMatchRuntimeState State = CainengGameRules::AdvanceMatchClock(
		CainengGameRules::StartMatch(Settings), 60.0f, Settings);
	TestTrue(TEXT("Objective phase is available after initial free play"), State.Phase == ECanergyMatchPhase::PublicObjective);
	TestTrue(TEXT("A time-limited goal can be offered"), CainengGameRules::TryStartPublicObjective(
		State, ECanergyPublicObjectiveKind::FeedCreature, 30.0f));
	State = CainengGameRules::AdvanceMatchClock(State, 30.0f, Settings);
	TestEqual(TEXT("An ignored task expires instead of blocking play"), static_cast<uint8>(State.ActiveObjective),
		static_cast<uint8>(ECanergyPublicObjectiveKind::None));
	TestTrue(TEXT("Timeout simply resolves the task; it does not end the round"), State.Phase == ECanergyMatchPhase::PublicObjective
		&& State.bPublicObjectiveResolved && State.ElapsedSeconds == 90.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanergyCarnivalMeterRulesTest,
	"Caineng.Gameplay.Core.CarnivalMeter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanergyCarnivalMeterRulesTest::RunTest(const FString& Parameters)
{
	FCanergyMatchRulesSettings Settings;
	FCanergyMatchRuntimeState State;
	TestEqual(TEXT("A waiting match cannot bank meter"),
		CainengGameRules::AddCarnivalMeter(State, 40.0f, Settings), 0.0f);

	State = CainengGameRules::StartMatch(Settings);
	CainengGameRules::AddCarnivalMeter(State, 65.0f, Settings);
	TestFalse(TEXT("Carnival does not start below its shared threshold"),
		CainengGameRules::CanStartCarnivalEvent(State, Settings));
	CainengGameRules::AddCarnivalMeter(State, 80.0f, Settings);
	TestTrue(TEXT("Meter saturates at its threshold"), FMath::IsNearlyEqual(State.CarnivalMeter, 100.0f));
	TestTrue(TEXT("Any player-earned threshold can start a shared event"),
		CainengGameRules::CanStartCarnivalEvent(State, Settings));
	TestTrue(TEXT("An event from the agreed pool starts"), CainengGameRules::TryStartCarnivalEvent(
		State, ECanergyCarnivalEventKind::FireworksParty, Settings));
	TestTrue(TEXT("Event duration is twenty-five seconds"), FMath::IsNearlyEqual(State.CarnivalRemainingSeconds, 25.0f));
	TestTrue(TEXT("Starting an event spends the meter"), FMath::IsNearlyZero(State.CarnivalMeter));
	TestFalse(TEXT("The same threshold cannot start a concurrent event"), CainengGameRules::TryStartCarnivalEvent(
		State, ECanergyCarnivalEventKind::LowGravity, Settings));

	State = CainengGameRules::AdvanceMatchClock(State, 24.9f, Settings);
	TestTrue(TEXT("Event remains visible before its duration ends"), State.IsCarnivalActive());
	State = CainengGameRules::AdvanceMatchClock(State, 0.1f, Settings);
	TestFalse(TEXT("The event ends and restores the base match state"), State.IsCarnivalActive());
	TestEqual(TEXT("Expired event identity is cleared"), static_cast<uint8>(State.ActiveCarnivalEvent),
		static_cast<uint8>(ECanergyCarnivalEventKind::None));

	State = CainengGameRules::AdvanceMatchClock(State, 1000.0f, Settings);
	TestEqual(TEXT("Finished match rejects new Joy-meter progress"),
		CainengGameRules::AddCarnivalMeter(State, 100.0f, Settings), 0.0f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
