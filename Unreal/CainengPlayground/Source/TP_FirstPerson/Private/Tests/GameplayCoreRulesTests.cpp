// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Variant_Shooter/Gameplay/InteractionEffectRules.h"
#include "Variant_Shooter/Gameplay/PaintSurfaceRules.h"
#include "Variant_Shooter/Gameplay/TraversalRules.h"
#include "Variant_Shooter/Gameplay/CelebrationRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanergyInteractionEffectRulesTest,
	"Caineng.Gameplay.Core.InteractionEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanergyInteractionEffectRulesTest::RunTest(const FString& Parameters)
{
	FCanergyInteractionEffectSpec BubbleSpec;
	BubbleSpec.Kind = ECanergyInteractionKind::Bubbled;
	BubbleSpec.DurationSeconds = 2.0f;
	BubbleSpec.Magnitude = 8.0f;
	BubbleSpec.MaxStacks = 2;
	BubbleSpec.StackPolicy = ECanergyEffectStackPolicy::AddStacks;

	FCanergyActiveInteractionEffect Active = CainengGameRules::ApplyInteractionEffect(FCanergyActiveInteractionEffect{}, BubbleSpec);
	TestTrue(TEXT("A valid non-lethal effect becomes active"), Active.IsActive());
	TestEqual(TEXT("New effect starts with one stack"), static_cast<int32>(Active.StackCount), 1);

	Active = CainengGameRules::ApplyInteractionEffect(Active, BubbleSpec);
	TestEqual(TEXT("AddStacks adds one stack"), static_cast<int32>(Active.StackCount), 2);
	Active = CainengGameRules::ApplyInteractionEffect(Active, BubbleSpec);
	TestEqual(TEXT("Stack count respects its configured cap"), static_cast<int32>(Active.StackCount), 2);

	FCanergyInteractionEffectSpec Longer = BubbleSpec;
	Longer.StackPolicy = ECanergyEffectStackPolicy::KeepLongest;
	Longer.DurationSeconds = 2.5f;
	Active.RemainingSeconds = 1.0f;
	Active = CainengGameRules::ApplyInteractionEffect(Active, Longer);
	TestTrue(TEXT("KeepLongest preserves the longer duration"), FMath::IsNearlyEqual(Active.RemainingSeconds, 2.5f));

	FCanergyInteractionEffectSpec Refreshed = Longer;
	Refreshed.StackPolicy = ECanergyEffectStackPolicy::RefreshDuration;
	Refreshed.DurationSeconds = 1.25f;
	Active = CainengGameRules::ApplyInteractionEffect(Active, Refreshed);
	TestTrue(TEXT("RefreshDuration resets duration to the incoming value"), FMath::IsNearlyEqual(Active.RemainingSeconds, 1.25f));

	FCanergyInteractionEffectSpec Capped = Refreshed;
	Capped.DurationSeconds = 20.0f;
	Active = CainengGameRules::ApplyInteractionEffect(FCanergyActiveInteractionEffect{}, Capped);
	TestTrue(TEXT("Control duration is capped at three seconds"), FMath::IsNearlyEqual(Active.RemainingSeconds, 3.0f));

	Active = CainengGameRules::TickInteractionEffect(Active, -2.0f);
	TestTrue(TEXT("Negative delta cannot speed up effect expiration"), FMath::IsNearlyEqual(Active.RemainingSeconds, 3.0f));
	Active = CainengGameRules::TickInteractionEffect(Active, 3.0f);
	TestFalse(TEXT("Expired interaction state is cleared"), Active.IsActive());
	TestEqual(TEXT("Expired state returns to None"), static_cast<uint8>(Active.Kind),
		static_cast<uint8>(ECanergyInteractionKind::None));

	TArray<FCanergyActiveInteractionEffect> ActiveEffects;
	BubbleSpec.DurationSeconds = 2.0f;
	ActiveEffects = CainengGameRules::ApplyInteractionEffect(ActiveEffects, BubbleSpec);
	FCanergyInteractionEffectSpec LightenedSpec = BubbleSpec;
	LightenedSpec.Kind = ECanergyInteractionKind::Lightened;
	LightenedSpec.DurationSeconds = 3.0f;
	ActiveEffects = CainengGameRules::ApplyInteractionEffect(ActiveEffects, LightenedSpec);
	TestEqual(TEXT("Different non-lethal effects can coexist"), ActiveEffects.Num(), 2);
	ActiveEffects = CainengGameRules::TickInteractionEffects(ActiveEffects, 2.1f);
	TestEqual(TEXT("Only the expired effect is removed"), ActiveEffects.Num(), 1);
	TestEqual(TEXT("The longer independent effect remains"), static_cast<uint8>(ActiveEffects[0].Kind),
		static_cast<uint8>(ECanergyInteractionKind::Lightened));

	FCanergyInteractionEffectSpec Invalid;
	Invalid.DurationSeconds = 1.0f;
	const FCanergyActiveInteractionEffect StillEmpty = CainengGameRules::ApplyInteractionEffect(FCanergyActiveInteractionEffect{}, Invalid);
	TestFalse(TEXT("An unspecified effect cannot create state"), StillEmpty.IsActive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanergyBubbleRespawnRulesTest,
	"Caineng.Gameplay.Core.BubbleRespawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanergyBubbleRespawnRulesTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Bubble respawn is unavailable before three seconds"),
		CainengGameRules::CanRespawnFromBubble(2.99f));
	TestTrue(TEXT("Bubble respawn is available at three seconds"),
		CainengGameRules::CanRespawnFromBubble(3.0f));
	TestTrue(TEXT("A configured zero delay is treated as immediately ready"),
		CainengGameRules::CanRespawnFromBubble(0.0f, 0.0f));
	TestFalse(TEXT("Negative time cannot trigger bubble respawn"),
		CainengGameRules::CanRespawnFromBubble(-0.1f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanergyJoyScoreRulesTest,
	"Caineng.Gameplay.Core.JoyScore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanergyJoyScoreRulesTest::RunTest(const FString& Parameters)
{
	static const ECanergyJoyAction RewardedActions[] = {
		ECanergyJoyAction::SurfaceTrail,
		ECanergyJoyAction::BounceChain,
		ECanergyJoyAction::Rescue,
		ECanergyJoyAction::PublicObjective,
		ECanergyJoyAction::TrickShot,
		ECanergyJoyAction::CarnivalParticipation,
		ECanergyJoyAction::RouteDiscovery,
		ECanergyJoyAction::EnvironmentalChain,
		ECanergyJoyAction::Collectible
	};
	for (ECanergyJoyAction Action : RewardedActions)
	{
		TestTrue(TEXT("Each intended playful behavior can award positive Joy Score"),
			CainengGameRules::GetJoyAward(Action) > 0);
	}
	TestEqual(TEXT("Unknown/no action awards no score"), CainengGameRules::GetJoyAward(ECanergyJoyAction::None), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanergySpraySurfaceCycleRulesTest,
	"Caineng.Gameplay.Core.SpraySurfaceCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanergySpraySurfaceCycleRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Liquid cycles to Bounce"),
		static_cast<uint8>(CainengGameRules::GetNextSpraySurfaceKind(ECanergySurfaceKind::Liquid)),
		static_cast<uint8>(ECanergySurfaceKind::Bounce));
	TestEqual(TEXT("Bounce cycles to Float"),
		static_cast<uint8>(CainengGameRules::GetNextSpraySurfaceKind(ECanergySurfaceKind::Bounce)),
		static_cast<uint8>(ECanergySurfaceKind::Float));
	TestEqual(TEXT("Float wraps to Liquid"),
		static_cast<uint8>(CainengGameRules::GetNextSpraySurfaceKind(ECanergySurfaceKind::Float)),
		static_cast<uint8>(ECanergySurfaceKind::Liquid));
	TestEqual(TEXT("Non-spray surfaces fall back to Liquid"),
		static_cast<uint8>(CainengGameRules::GetNextSpraySurfaceKind(ECanergySurfaceKind::Sticky)),
		static_cast<uint8>(ECanergySurfaceKind::Liquid));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanergyTraversalContactRulesTest,
	"Caineng.Gameplay.Core.TraversalContact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanergyTraversalContactRulesTest::RunTest(const FString& Parameters)
{
	using namespace CainengGameRules;
	FTraversalContactState State;
	TestFalse(TEXT("Airborne probe above bounce pad does not consume landing"),
		AdvanceTraversalContact(State, false, true, false, 0.016f));
	TestTrue(TEXT("Landing after airborne probe launches"),
		AdvanceTraversalContact(State, true, true, false, 0.016f));
	TestFalse(TEXT("Same grounded contact does not relaunch every frame"),
		AdvanceTraversalContact(State, true, true, false, 0.016f));
	AdvanceTraversalContact(State, false, true, false, 0.016f);
	TestTrue(TEXT("Returning to the pad permits another bounce"),
		AdvanceTraversalContact(State, true, true, false, 0.016f));
	AdvanceTraversalContact(State, true, false, true, 0.016f);
	AdvanceTraversalContact(State, false, false, false, 0.5f);
	TestTrue(TEXT("Float survives leaving the short ground probe"),
		FMath::IsNearlyEqual(State.FloatSecondsRemaining, 1.0f));
	AdvanceTraversalContact(State, false, false, true, -1.0f);
	TestTrue(TEXT("Airborne proximity and negative delta cannot replenish float"),
		FMath::IsNearlyEqual(State.FloatSecondsRemaining, 1.0f));
	AdvanceTraversalContact(State, false, false, true, 2.0f);
	TestEqual(TEXT("Float expires even while hovering above its pad"), State.FloatSecondsRemaining, 0.0f);
	AdvanceTraversalContact(State, true, false, true, 0.016f);
	AdvanceTraversalContact(State, true, false, false, 0.016f);
	TestEqual(TEXT("Landing on ordinary ground clears float"), State.FloatSecondsRemaining, 0.0f);
	AdvanceTraversalContact(State, true, false, true, 0.016f);
	TestFalse(TEXT("Bubble state prevents launch and clears traversal"),
		AdvanceTraversalContact(State, true, true, false, 0.016f, true));
	TestEqual(TEXT("Bubble clears stored glide time"), State.FloatSecondsRemaining, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanergyLiquidDiveRulesTest,
	"Caineng.Gameplay.Core.LiquidDive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanergyLiquidDiveRulesTest::RunTest(const FString& Parameters)
{
	using namespace CainengGameRules;
	TestTrue(TEXT("Held input on grounded liquid enables dive"), CanLiquidDive(true, true, true, false));
	TestFalse(TEXT("Releasing input exits dive"), CanLiquidDive(false, true, true, false));
	TestFalse(TEXT("Airborne proximity is not ground diving"), CanLiquidDive(true, false, true, false));
	TestFalse(TEXT("Unpainted ground cannot enable dive"), CanLiquidDive(true, true, false, false));
	TestFalse(TEXT("Bubble/death cancels dive"), CanLiquidDive(true, true, true, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanergyWallSurfaceRulesTest,
	"Caineng.Gameplay.Core.WallSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanergyWallSurfaceRulesTest::RunTest(const FString& Parameters)
{
	using namespace CainengGameRules;
	TestTrue(TEXT("Liquid vertical wall supports climbing"), IsClimbableLiquidWall(true, 0.0f));
	TestTrue(TEXT("Slightly tilted wall is allowed"), IsClimbableLiquidWall(true, 0.2f));
	TestFalse(TEXT("Ordinary wall cannot attach"), IsClimbableLiquidWall(false, 0.0f));
	TestFalse(TEXT("Liquid floor cannot masquerade as wall"), IsClimbableLiquidWall(true, 1.0f));
	TestFalse(TEXT("Ceiling cannot become unlimited traversal"), IsClimbableLiquidWall(true, -1.0f));
	TestFalse(TEXT("Steep ramp stays regular ground traversal"), IsClimbableLiquidWall(true, 0.5f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCanergyCelebrationRulesTest,
	"Caineng.Gameplay.Core.Celebration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCanergyCelebrationRulesTest::RunTest(const FString& Parameters)
{
	using namespace CainengGameRules;
	TMap<ECanergyJoyAction, int32> Contributions;
	TestEqual(TEXT("No points still receives a positive explorer title"), FString(CelebrationName(SelectCelebration(Contributions))), FString(TEXT("自在游乐家")));
	Contributions.Add(ECanergyJoyAction::Rescue, 12);
	Contributions.Add(ECanergyJoyAction::SurfaceTrail, 9);
	TestTrue(TEXT("Title reflects strongest actual contribution"), SelectCelebration(Contributions) == ECanergyJoyAction::Rescue);
	Contributions.Add(ECanergyJoyAction::SurfaceTrail, 12);
	TestTrue(TEXT("Ties deterministic regardless of map iteration order"), SelectCelebration(Contributions) == ECanergyJoyAction::SurfaceTrail);
	Contributions.Add(ECanergyJoyAction::None, 999);
	Contributions.Add(ECanergyJoyAction::BounceChain, -99);
	TestTrue(TEXT("Unknown and negative contributions cannot win"), SelectCelebration(Contributions) == ECanergyJoyAction::SurfaceTrail);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
