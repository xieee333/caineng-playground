#pragma once
#include "GameplayTypes.h"

namespace CainengGameRules
{
	inline ECanergyJoyAction SelectCelebration(const TMap<ECanergyJoyAction, int32>& Contributions)
	{
		ECanergyJoyAction Best = ECanergyJoyAction::None;
		int32 BestPoints = 0;
		for (const auto& Pair : Contributions)
		{
			if (Pair.Key == ECanergyJoyAction::None) continue;
			if (Pair.Value > BestPoints || (Pair.Value == BestPoints && Pair.Value > 0
				&& static_cast<uint8>(Pair.Key) < static_cast<uint8>(Best)))
			{
				Best = Pair.Key; BestPoints = Pair.Value;
			}
		}
		return Best;
	}

	inline const TCHAR* CelebrationName(ECanergyJoyAction Action)
	{
		switch (Action)
		{
		case ECanergyJoyAction::SurfaceTrail: return TEXT("彩能铺路师");
		case ECanergyJoyAction::BounceChain: return TEXT("空中杂技家");
		case ECanergyJoyAction::Rescue: return TEXT("最佳救场");
		case ECanergyJoyAction::PublicObjective: return TEXT("游乐场建设者");
		case ECanergyJoyAction::TrickShot: return TEXT("花式玩家");
		case ECanergyJoyAction::CarnivalParticipation: return TEXT("狂欢达人");
		case ECanergyJoyAction::RouteDiscovery: return TEXT("路线探索家");
		case ECanergyJoyAction::EnvironmentalChain: return TEXT("连锁反应家");
		case ECanergyJoyAction::Collectible: return TEXT("寻宝专家");
		default: return TEXT("自在游乐家");
		}
	}
}
