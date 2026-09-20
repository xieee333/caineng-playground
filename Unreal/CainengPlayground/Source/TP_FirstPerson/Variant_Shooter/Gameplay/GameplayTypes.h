// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTypes.generated.h"

UENUM(BlueprintType)
enum class ECanergySurfaceKind : uint8
{
	None UMETA(DisplayName = "无"),
	Liquid UMETA(DisplayName = "液态"),
	Bounce UMETA(DisplayName = "弹跳"),
	Float UMETA(DisplayName = "飘浮"),
	Sticky UMETA(DisplayName = "黏性"),
	Mirror UMETA(DisplayName = "镜面"),
	Conductive UMETA(DisplayName = "传导")
};

UENUM(BlueprintType)
enum class ECanergyInteractionKind : uint8
{
	None UMETA(DisplayName = "无"),
	Knockback UMETA(DisplayName = "击退"),
	Pull UMETA(DisplayName = "吸附"),
	Frozen UMETA(DisplayName = "短暂冻结"),
	Tethered UMETA(DisplayName = "短暂绑缚"),
	Bubbled UMETA(DisplayName = "泡泡化"),
	Lightened UMETA(DisplayName = "变轻"),
	Swapped UMETA(DisplayName = "位置交换"),
	ObjectStolen UMETA(DisplayName = "道具掉落")
};

UENUM(BlueprintType)
enum class ECanergyEffectStackPolicy : uint8
{
	Replace UMETA(DisplayName = "替换"),
	RefreshDuration UMETA(DisplayName = "刷新持续时间"),
	KeepLongest UMETA(DisplayName = "保留较长时间"),
	AddStacks UMETA(DisplayName = "叠层")
};

USTRUCT(BlueprintType)
struct FCanergyInteractionEffectSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canergy|Effect")
	ECanergyInteractionKind Kind = ECanergyInteractionKind::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canergy|Effect")
	ECanergyEffectStackPolicy StackPolicy = ECanergyEffectStackPolicy::RefreshDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canergy|Effect", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float DurationSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canergy|Effect", meta = (ClampMin = "0.0"))
	float Magnitude = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Canergy|Effect", meta = (ClampMin = "1", ClampMax = "8"))
	uint8 MaxStacks = 1;
};

USTRUCT(BlueprintType)
struct FCanergyActiveInteractionEffect
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Canergy|Effect")
	ECanergyInteractionKind Kind = ECanergyInteractionKind::None;

	UPROPERTY(BlueprintReadOnly, Category = "Canergy|Effect")
	float RemainingSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Canergy|Effect")
	float Magnitude = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Canergy|Effect")
	uint8 StackCount = 0;

	bool IsActive() const
	{
		return Kind != ECanergyInteractionKind::None && RemainingSeconds > 0.0f;
	}
};

/** Only positive play behaviors are scoreable; damage/eliminations are deliberately absent. */
UENUM(BlueprintType)
enum class ECanergyJoyAction : uint8
{
	None UMETA(DisplayName = "无"),
	SurfaceTrail UMETA(DisplayName = "彩能铺路"),
	BounceChain UMETA(DisplayName = "弹跳连锁"),
	Rescue UMETA(DisplayName = "救援"),
	PublicObjective UMETA(DisplayName = "公共目标"),
	TrickShot UMETA(DisplayName = "花式命中"),
	CarnivalParticipation UMETA(DisplayName = "参与狂欢"),
	RouteDiscovery UMETA(DisplayName = "发现路线"),
	EnvironmentalChain UMETA(DisplayName = "环境连锁"),
	Collectible UMETA(DisplayName = "收集彩泡")
};

UENUM(BlueprintType)
enum class ECanergyMatchPhase : uint8
{
	Waiting UMETA(DisplayName = "等待开始"),
	FreePlay UMETA(DisplayName = "自由游玩"),
	PublicObjective UMETA(DisplayName = "公共目标"),
	Finished UMETA(DisplayName = "结算")
};

UENUM(BlueprintType)
enum class ECanergyPublicObjectiveKind : uint8
{
	None UMETA(DisplayName = "无"),
	PaintMonument UMETA(DisplayName = "为巨型雕像上色"),
	FeedCreature UMETA(DisplayName = "帮助中立生物收集食物"),
	BalloonChase UMETA(DisplayName = "追逐奖励气球"),
	ActivateSprayer UMETA(DisplayName = "启动喷射装置")
};

UENUM(BlueprintType)
enum class ECanergyCarnivalEventKind : uint8
{
	None UMETA(DisplayName = "无"),
	LowGravity UMETA(DisplayName = "低重力"),
	ColorStorm UMETA(DisplayName = "彩能狂喷"),
	BalloonRain UMETA(DisplayName = "气球雨"),
	GiantCreature UMETA(DisplayName = "巨型生物"),
	RotatingScene UMETA(DisplayName = "场景旋转"),
	FireworksParty UMETA(DisplayName = "烟花派对")
};

USTRUCT(BlueprintType)
struct FCanergyMatchRulesSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Match", meta = (ClampMin = "60.0"))
	float RoundDurationSeconds = 420.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Match", meta = (ClampMin = "0.0"))
	float InitialFreePlaySeconds = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Match", meta = (ClampMin = "10.0"))
	float PublicObjectivePhaseSeconds = 210.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Match", meta = (ClampMin = "0.0"))
	float RepeatFreePlaySeconds = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Match", meta = (ClampMin = "5.0", ClampMax = "30.0"))
	float CarnivalEventDurationSeconds = 25.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Canergy|Match", meta = (ClampMin = "1.0"))
	float CarnivalMeterThreshold = 100.0f;
};

USTRUCT(BlueprintType)
struct FCanergyMatchRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Canergy|Match")
	ECanergyMatchPhase Phase = ECanergyMatchPhase::Waiting;

	UPROPERTY(BlueprintReadOnly, Category = "Canergy|Match")
	float ElapsedSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Canergy|Match")
	float CarnivalMeter = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Canergy|Match")
	float PhaseRemainingSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Canergy|Match")
	float ActiveObjectiveRemainingSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Canergy|Match")
	float CarnivalRemainingSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Canergy|Match")
	ECanergyPublicObjectiveKind ActiveObjective = ECanergyPublicObjectiveKind::None;

	UPROPERTY(BlueprintReadOnly, Category = "Canergy|Match")
	ECanergyCarnivalEventKind ActiveCarnivalEvent = ECanergyCarnivalEventKind::None;

	UPROPERTY(BlueprintReadOnly, Category = "Canergy|Match")
	bool bPublicObjectiveResolved = false;

	bool IsCarnivalActive() const
	{
		return ActiveCarnivalEvent != ECanergyCarnivalEventKind::None && CarnivalRemainingSeconds > 0.0f;
	}
};
