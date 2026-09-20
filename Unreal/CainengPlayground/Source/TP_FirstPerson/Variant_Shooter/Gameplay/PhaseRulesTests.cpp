#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PhaseRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanergyPhaseRulesTest, "Caineng.Gameplay.Phase.Rules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanergyPhaseRulesTest::RunTest(const FString& Parameters)
{
	using namespace CainengGameRules;
	FPhaseState State;
	TestFalse(TEXT("Disabled states such as carrying or bubbling are rejected"), BeginPhase(State, true, true, 60, 180, true));
	TestFalse(TEXT("Ordinary walls are never phaseable"), BeginPhase(State, false, false, 60, 180, true));
	TestFalse(TEXT("A blocked exit is rejected"), BeginPhase(State, false, true, 60, 180, false));
	TestFalse(TEXT("Thick geometry is rejected"), BeginPhase(State, false, true, 240, 180, true));
	TestTrue(TEXT("A tagged thin wall with a safe exit can be crossed"), BeginPhase(State, false, true, 60, 180, true));
	TestFalse(TEXT("Cooldown prevents immediate reuse"), CanBeginPhase(State, false, true, 60, 180, true));
	AdvancePhase(State, 0.5f);
	TestTrue(TEXT("Phase remains active during its window"), State.ActiveRemaining > 0.0f);
	TestTrue(TEXT("Attacking or taking a hit breaks concealment"), BreakPhase(State));
	TestEqual(TEXT("Break only ends concealment"), State.ActiveRemaining, 0.0f);
	TestTrue(TEXT("Cooldown remains after a break"), State.CooldownRemaining > 0.0f);
	AdvancePhase(State, 10.0f);
	TestTrue(TEXT("Ability becomes reusable after cooldown"), CanBeginPhase(State, false, true, 60, 180, true));
	return true;
}
#endif
