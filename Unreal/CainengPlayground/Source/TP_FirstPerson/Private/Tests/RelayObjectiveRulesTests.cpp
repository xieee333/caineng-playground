// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Variant_Shooter/RelayObjectiveRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRelaySequenceRulesTest,
	"Caineng.Gameplay.RelaySequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRelaySequenceRulesTest::RunTest(const FString& Parameters)
{
	FRelayHitOutcome Outcome = CainengGameRules::ResolveRelayHit(0, 0);
	TestEqual(TEXT("Cyan advances to 1/3"), Outcome.NewProgress, 1);
	TestEqual(TEXT("Cyan hit is accepted"), static_cast<uint8>(Outcome.Resolution),
		static_cast<uint8>(ERelayHitResolution::Advanced));

	Outcome = CainengGameRules::ResolveRelayHit(1, 1);
	TestEqual(TEXT("Orange advances to 2/3"), Outcome.NewProgress, 2);
	Outcome = CainengGameRules::ResolveRelayHit(2, 2);
	TestEqual(TEXT("Magenta advances to 3/3"), Outcome.NewProgress, 3);
	TestEqual(TEXT("Final relay is accepted"), static_cast<uint8>(Outcome.Resolution),
		static_cast<uint8>(ERelayHitResolution::Advanced));

	Outcome = CainengGameRules::ResolveRelayHit(0, 2);
	TestEqual(TEXT("Out-of-order magenta resets progress"), Outcome.NewProgress, 0);
	TestEqual(TEXT("Out-of-order result is reported"), static_cast<uint8>(Outcome.Resolution),
		static_cast<uint8>(ERelayHitResolution::WrongOrder));

	Outcome = CainengGameRules::ResolveRelayHit(1, 0);
	TestEqual(TEXT("Previously calibrated cyan does not erase progress"), Outcome.NewProgress, 1);
	TestEqual(TEXT("Previously calibrated target is identified"), static_cast<uint8>(Outcome.Resolution),
		static_cast<uint8>(ERelayHitResolution::AlreadyCalibrated));

	Outcome = CainengGameRules::ResolveRelayHit(2, 3);
	TestEqual(TEXT("Invalid target leaves progress unchanged"), Outcome.NewProgress, 2);
	TestEqual(TEXT("Invalid target is rejected"), static_cast<uint8>(Outcome.Resolution),
		static_cast<uint8>(ERelayHitResolution::Invalid));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalibrationHurdleRulesTest,
	"Caineng.Gameplay.CalibrationHurdle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCalibrationHurdleRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Airborne forward crossing inside lane succeeds"),
		CainengGameRules::CrossedCalibrationHurdle(-1.0f, 1.0f, 0.0f, true));
	TestFalse(TEXT("Walking across does not satisfy the hurdle"),
		CainengGameRules::CrossedCalibrationHurdle(-1.0f, 1.0f, 0.0f, false));
	TestFalse(TEXT("Crossing outside the lane does not satisfy the hurdle"),
		CainengGameRules::CrossedCalibrationHurdle(-1.0f, 1.0f, 186.0f, true));
	TestFalse(TEXT("Already being past the hurdle is not a crossing"),
		CainengGameRules::CrossedCalibrationHurdle(1.0f, 2.0f, 0.0f, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRelayTrialExitRulesTest,
	"Caineng.Gameplay.TrialExit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRelayTrialExitRulesTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Exit cannot complete before all relays are calibrated"),
		CainengGameRules::CanCompleteRelayTrial(2, true, 0.0f));
	TestFalse(TEXT("Exit cannot complete before the airborne hurdle crossing"),
		CainengGameRules::CanCompleteRelayTrial(3, false, 0.0f));
	TestTrue(TEXT("Completed route succeeds inside the exit radius"),
		CainengGameRules::CanCompleteRelayTrial(3, true, FMath::Square(239.0f)));
	TestTrue(TEXT("Exit boundary is inclusive"),
		CainengGameRules::CanCompleteRelayTrial(3, true, FMath::Square(240.0f)));
	TestFalse(TEXT("Exit does not complete outside the radius"),
		CainengGameRules::CanCompleteRelayTrial(3, true, FMath::Square(241.0f)));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
