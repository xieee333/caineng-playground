// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTypes.h"

namespace CainengGameRules
{
	inline ECanergySurfaceKind GetNextSpraySurfaceKind(ECanergySurfaceKind Current)
	{
		switch (Current)
		{
		case ECanergySurfaceKind::Liquid: return ECanergySurfaceKind::Bounce;
		case ECanergySurfaceKind::Bounce: return ECanergySurfaceKind::Float;
		case ECanergySurfaceKind::Float: return ECanergySurfaceKind::Liquid;
		default: return ECanergySurfaceKind::Liquid;
		}
	}
}
