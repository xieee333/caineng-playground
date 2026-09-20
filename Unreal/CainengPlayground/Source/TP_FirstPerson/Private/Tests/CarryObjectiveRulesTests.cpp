#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Variant_Shooter/Gameplay/CarryObjectiveRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCarryOwnershipTest, "Caineng.Gameplay.Carry.Ownership",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarryOwnershipTest::RunTest(const FString& Parameters)
{
	using namespace CainengGameRules;
	FCarryObjectiveState State;
	TestFalse(TEXT("Bubble/ineligible cannot take"), TryTakeCore(State, 0, 0, false, true));
	TestFalse(TEXT("Cannot take through distance/wall"), TryTakeCore(State, 0, 0, true, false));
	TestFalse(TEXT("Invalid team"), TryTakeCore(State, 0, 2, true, true));
	TestTrue(TEXT("First carrier takes"), TryTakeCore(State, 0, 0, true, true));
	TestFalse(TEXT("Second claimant cannot steal ownership"), TryTakeCore(State, 1, 1, true, true));
	TestFalse(TEXT("Non-owner cannot drop"), DropCore(State, 1));
	TestFalse(TEXT("Wrong goal cannot score"), TryDeliverCore(State, 0, 1, true));
	TestFalse(TEXT("Remote delivery cannot score"), TryDeliverCore(State, 0, 0, false));
	TestTrue(TEXT("Owner can throw/drop"), DropCore(State, 0));
	TestFalse(TEXT("Pickup protected during release"), TryTakeCore(State, 1, 1, true, true));
	AdvanceCarryClock(State, 0.7f);
	TestTrue(TEXT("Opponent may intercept"), TryTakeCore(State, 1, 1, true, true));
	TestTrue(TEXT("Correct carrier scores once"), TryDeliverCore(State, 1, 1, true));
	TestFalse(TEXT("Duplicate delivery rejected"), TryDeliverCore(State, 1, 1, true));
	TestEqual(TEXT("Only defending team's score changed"), State.Scores[0], 0);
	TestEqual(TEXT("Exactly one point"), State.Scores[1], 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCarryRoundTest, "Caineng.Gameplay.Carry.Round",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarryRoundTest::RunTest(const FString& Parameters)
{
	using namespace CainengGameRules;
	FCarryObjectiveState State;
	for (int32 I=0; I<3; ++I)
	{
		AdvanceCarryClock(State, 2.1f);
		TestTrue(TEXT("Take next core"), TryTakeCore(State, 0, 0, true, true));
		TestTrue(TEXT("Deliver next core"), TryDeliverCore(State, 0, 0, true));
	}
	TestTrue(TEXT("Three points end match"), State.bFinished);
	TestFalse(TEXT("No pickup after finish"), TryTakeCore(State, 1, 1, true, true));
	State = FCarryObjectiveState();
	AdvanceCarryClock(State, -1.0f);
	TestEqual(TEXT("Negative time ignored"), State.Remaining, 180.0f);
	AdvanceCarryClock(State, 180.0f);
	TestTrue(TEXT("Timeout ends even at draw"), State.bFinished);
	TestFalse(TEXT("No scoring at timeout"), TryDeliverCore(State, 0, 0, true));
	State = FCarryObjectiveState();
	TestTrue(TEXT("New round resets ownership and score"), State.Carrier == INDEX_NONE && State.Scores[0] == 0 && !State.bFinished);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCarryProtectionTest, "Caineng.Gameplay.Carry.Protection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarryProtectionTest::RunTest(const FString& Parameters)
{
	using namespace CainengGameRules;
	TestFalse(TEXT("Dead cannot contest"), CanContestCore(true, false, 0.0f));
	TestFalse(TEXT("Bubble cannot contest"), CanContestCore(false, true, 0.0f));
	TestFalse(TEXT("Fresh respawn cannot contest"), CanContestCore(false, false, 1.99f));
	TestTrue(TEXT("Protection expiry restores contest"), CanContestCore(false, false, 0.0f));
	TestFalse(TEXT("Invalid protection value stays safe"), CanContestCore(false, false, NAN));
	return true;
}
#endif
