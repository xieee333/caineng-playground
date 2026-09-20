// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/ShooterGameMode.h"
#include "ShooterUI.h"
#include "UI/RelayObjectiveWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "CollisionQueryParams.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ShooterCharacter.h"
#include "ShooterPlayerController.h"
#include "RelayObjectiveRules.h"
#include "Variant_Shooter/Gameplay/MatchLoopRules.h"
#include "Variant_Shooter/Gameplay/CanergyRuntimeComponents.h"
#include "Variant_Shooter/Gameplay/TraversalProbe.h"
#include "Variant_Shooter/Gameplay/PlaygroundBotDirector.h"
#include "Variant_Shooter/Gameplay/CelebrationRules.h"
#include "Variant_Shooter/Gameplay/CarryPrototype.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
	float FindGroundZ(UWorld* World, const FVector& Position, const AActor* IgnoreActor)
	{
		FHitResult Hit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(CainengGroundProbe), false, IgnoreActor);
		const FVector TraceStart = Position + FVector::UpVector * 1200.0f;
		const FVector TraceEnd = Position - FVector::UpVector * 2000.0f;
		return World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams)
			? Hit.ImpactPoint.Z
			: Position.Z - 96.0f;
	}

	AStaticMeshActor* SpawnColoredBlock(UWorld* World, UStaticMesh* Mesh, UMaterialInterface* BaseMaterial,
		const FVector& Location, const FRotator& Rotation, const FVector& Scale, const FLinearColor& Color, FName Tag)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Block = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FTransform(Rotation, Location), SpawnParams);
	if (!Block) return nullptr;
	Block->SetReplicates(true);

		UStaticMeshComponent* MeshComponent = Block->GetStaticMeshComponent();
		MeshComponent->SetMobility(EComponentMobility::Movable);
		MeshComponent->SetStaticMesh(Mesh);
		Block->SetActorScale3D(Scale);
		Block->Tags.Add(Tag);
		if (BaseMaterial)
		{
			MeshComponent->SetMaterial(0, BaseMaterial);
			if (UMaterialInstanceDynamic* DynamicMaterial = MeshComponent->CreateAndSetMaterialInstanceDynamic(0))
			{
				DynamicMaterial->SetVectorParameterValue(FName("Color"), Color);
				DynamicMaterial->SetVectorParameterValue(FName("BaseColor"), Color);
				DynamicMaterial->SetVectorParameterValue(FName("EmissiveColor"), Color * 0.35f);
			}
		}
		return Block;
	}
}

AShooterGameMode::AShooterGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	GameStateClass = ACanergyMatchGameState::StaticClass();
}

void AShooterGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (CarryPrototype)
	{
		MatchRuntimeState.Phase = CarryPrototype->IsFinished() ? ECanergyMatchPhase::Finished : ECanergyMatchPhase::FreePlay;
		UpdateReplicatedMatchState();
		return;
	}
	if (!HasAuthority() || MatchRuntimeState.Phase == ECanergyMatchPhase::Waiting) return;

	const FCanergyMatchRuntimeState PreviousState = MatchRuntimeState;
	MatchRuntimeState = CainengGameRules::AdvanceMatchClock(MatchRuntimeState, DeltaSeconds, MatchRulesSettings);
	if (PreviousState.Phase != ECanergyMatchPhase::PublicObjective
		&& MatchRuntimeState.Phase == ECanergyMatchPhase::PublicObjective)
	{
		bObjectiveSpawnedThisPhase = true;
		if (CainengGameRules::TryStartPublicObjective(MatchRuntimeState,
			ECanergyPublicObjectiveKind::PaintMonument, 90.0f))
		{
			SpawnPublicObjective();
			if (RelayObjectiveWidget)
			{
				RelayObjectiveWidget->SetGameplayStatus(FText::FromString(TEXT(
					"公共目标 · 给三块纪念碑碎片喷上彩能\n可参加，也可以继续自由游玩\nWASD 移动 · 空格跳跃 · Shift 冲刺 · 鼠标观察\n左键喷绘/开火 · R 换彩能 · Q 换玩具\nZ 选技能 · F 发动 · E 武器特殊\nCtrl 潜行/上墙 · 墙上空格蹬跳")));
			}
			ShowPlayerFeedback(FText::FromString(TEXT("公共喷绘目标出现 · 三块碎片都被点亮即可完成")), 3.0f);
		}
	}

	if (PreviousState.ActiveObjective != ECanergyPublicObjectiveKind::None
		&& MatchRuntimeState.ActiveObjective == ECanergyPublicObjectiveKind::None)
	{
		const bool bCompleted = PublicObjectivePaintedActors.Num() >= PublicObjectiveActors.Num()
			&& !PublicObjectiveActors.IsEmpty();
		ClearPublicObjective();
		if (RelayObjectiveWidget)
		{
			RelayObjectiveWidget->SetGameplayStatus(FText::FromString(TEXT(
				"自由游玩 · 喷绘地形，尝试弹跳/滑翔路线\n公共目标、狂欢事件可参加，也可自由探索\nWASD 移动 · 空格跳跃 · Shift 冲刺 · 鼠标观察\n左键喷绘/开火 · R 换彩能 · Q 换玩具\nZ 选技能 · F 发动 · E 武器特殊\nCtrl 潜行/上墙 · 墙上空格蹬跳")));
		}
		ShowPlayerFeedback(FText::FromString(bCompleted
			? TEXT("公共目标完成！参与者获得欢乐值。")
			: TEXT("公共目标时间结束 · 没有惩罚，继续自由游玩。")), 3.0f);
	}

	if (PreviousState.ActiveCarnivalEvent != ECanergyCarnivalEventKind::None
		&& MatchRuntimeState.ActiveCarnivalEvent == ECanergyCarnivalEventKind::None)
	{
		ApplyCarnivalEvent(ECanergyCarnivalEventKind::None);
		ShowPlayerFeedback(FText::FromString(TEXT("狂欢事件结束 · 欢乐槽正在重新累积")), 2.5f);
	}

	if (PreviousState.Phase != ECanergyMatchPhase::Finished
		&& MatchRuntimeState.Phase == ECanergyMatchPhase::Finished)
	{
		ClearPublicObjective();
		ApplyCarnivalEvent(ECanergyCarnivalEventKind::None);
		if (RelayObjectiveWidget)
		{
			TArray<AActor*> Participants;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShooterCharacter::StaticClass(), Participants);
			FString Results = TEXT("本场结束 · 每个人都有自己的精彩\n");
			int32 BotNumber = 0;
			for (AActor* Actor : Participants)
			{
				const auto* Character = Cast<AShooterCharacter>(Actor);
				const auto* Joy = Character->GetCanergyJoyScore();
				const FString Name = Character->IsPlayerControlled() ? TEXT("你") : FString::Printf(TEXT("游乐伙伴 %d"), ++BotNumber);
				const TCHAR* Title = CainengGameRules::CelebrationName(CainengGameRules::SelectCelebration(Joy->GetJoyContributions()));
				Results += FString::Printf(TEXT("%s · %s · 欢乐值 %d\n"), *Name, Title, Joy->GetJoyScore());
				UE_LOG(LogTemp, Display, TEXT("CANERGY_RESULTS participant=%s title=%s joy=%d"), *Name, Title, Joy->GetJoyScore());
			}
			Results += TEXT("回车再开一局 · 自由探索也值得一个称号");
			RelayObjectiveWidget->SetGameplayStatus(FText::FromString(Results));
			UE_LOG(LogTemp, Display, TEXT("CANERGY_RESULTS complete participants=%d elapsed=%.1f"), Participants.Num(), MatchRuntimeState.ElapsedSeconds);
		}
	}

	if (CainengGameRules::CanStartCarnivalEvent(MatchRuntimeState, MatchRulesSettings))
	{
		static const ECanergyCarnivalEventKind Events[] =
		{
			ECanergyCarnivalEventKind::LowGravity,
			ECanergyCarnivalEventKind::ColorStorm,
			ECanergyCarnivalEventKind::BalloonRain
		};
		const ECanergyCarnivalEventKind Event = Events[CarnivalEventCursor % UE_ARRAY_COUNT(Events)];
		++CarnivalEventCursor;
		if (CainengGameRules::TryStartCarnivalEvent(MatchRuntimeState, Event, MatchRulesSettings))
		{
			ApplyCarnivalEvent(Event);
		}
	}

	UpdateReplicatedMatchState();
}

void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();
#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("CanergyQuickMatchProbe")))
	{
		MatchRulesSettings.RoundDurationSeconds = 20.0f;
		MatchRulesSettings.InitialFreePlaySeconds = 5.0f;
		MatchRulesSettings.PublicObjectivePhaseSeconds = 10.0f;
	}
#endif
	MatchRuntimeState = CainengGameRules::StartMatch(MatchRulesSettings);
#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("CanergyTraversalProbe")))
	{
		GetWorld()->SpawnActor<ACanergyTraversalProbe>();
	}
#endif
	// Keep deterministic traversal probes isolated; normal local play gets five peers.
	if (!FParse::Param(FCommandLine::Get(), TEXT("CanergyTraversalProbe")))
		GetWorld()->SpawnActor<ACanergyPlaygroundBotDirector>();

	// create the UI
	if ((RelayObjectiveWidget = CreateWidget<URelayObjectiveWidget>(UGameplayStatics::GetPlayerController(GetWorld(), 0))))
	{
		RelayObjectiveWidget->AddToViewport(5);
		RelayObjectiveWidget->SetGameplayStatus(FText::FromString(TEXT(
			"自由游玩 · 喷绘地形，尝试弹跳/滑翔路线\n公共目标、狂欢事件可参加，也可自由探索\nWASD 移动 · 空格跳跃 · Shift 冲刺 · 鼠标观察\n左键喷绘/开火 · R 换彩能 · Q 换玩具\nZ 选技能 · F 发动 · E 武器特殊\nCtrl 潜行/上墙 · 墙上空格蹬跳")));
	}

	// create each additional local player.
	// Player 0 will be created automatically as part of regular game init
	for (int32 i = 2; i <= NumberOfLocalPlayers; ++i)
	{
		if (AShooterPlayerController* NewPlayer = Cast<AShooterPlayerController>(UGameplayStatics::CreatePlayer(GetWorld(), -1, true)))
		{
			NewPlayer->SetTeam(1 - (i % 2));
		}
	}

	// The template map contains several starts for local multiplayer. Anchor the
	// solo traversal route to the same Player0 start that ChoosePlayerStart uses.
	TArray<AActor*> PrototypePlayerStarts;
	UGameplayStatics::GetAllActorsOfClassWithTag(
		GetWorld(), APlayerStart::StaticClass(), FName("Player0"), PrototypePlayerStarts);
	AActor* PlayerStart = PrototypePlayerStarts.IsEmpty()
		? UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass())
		: PrototypePlayerStarts[0];
	if (PlayerStart)
	{
		MatchAnchorLocation = PlayerStart->GetActorLocation();
		MatchForwardDirection = PlayerStart->GetActorForwardVector().GetSafeNormal2D();
		MatchRightDirection = PlayerStart->GetActorRightVector().GetSafeNormal2D();
		if (!FParse::Param(FCommandLine::Get(), TEXT("CanergyCarryPrototype"))) SpawnColorTraversalTrack(PlayerStart);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("CanergyCarryPrototype")))
	{
		CarryPrototype = GetWorld()->SpawnActor<ACanergyCarryPrototype>();
		SetPrototypeStatus(FText::FromString(TEXT("正在准备核心争夺灰盒 · 1 名玩家 + 5 个机器人")));
	}
	UpdateReplicatedMatchState();
}

void AShooterGameMode::SetPrototypeStatus(const FText& Status)
{
	if (RelayObjectiveWidget) RelayObjectiveWidget->SetGameplayStatus(Status);
}

void AShooterGameMode::UpdateRelayObjectiveWidget()
{
	if (RelayObjectiveWidget)
	{
		RelayObjectiveWidget->SetObjectiveState(RelayProgress, bJumpedCalibrationHurdle, bReachedExit);
	}
}

void AShooterGameMode::ShowPlayerFeedback(const FText& Feedback, float Duration)
{
	if (RelayObjectiveWidget)
	{
		RelayObjectiveWidget->ShowFeedback(Feedback, Duration);
	}
}

void AShooterGameMode::UpdateReplicatedMatchState()
{
	if (ACanergyMatchGameState* MatchGameState = GetWorld()->GetGameState<ACanergyMatchGameState>())
	{
		MatchGameState->SetCanergyMatchStateAuthorityOnly(MatchRuntimeState);
	}
}

void AShooterGameMode::SpawnPublicObjective()
{
	ClearPublicObjective();
	UStaticMesh* MonumentShardMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!MonumentShardMesh) return;

	const FRotator Facing(0.0f, MatchForwardDirection.Rotation().Yaw, 0.0f);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FVector Location = MatchAnchorLocation + MatchForwardDirection * 1500.0f
			+ MatchRightDirection * ((Index - 1) * 260.0f);
		Location.Z = FindGroundZ(GetWorld(), Location, nullptr) + 130.0f;
		if (AStaticMeshActor* Shard = SpawnColoredBlock(GetWorld(), MonumentShardMesh, BaseMaterial,
			Location, Facing, FVector(0.62f, 0.22f, 0.82f), FLinearColor(0.18f, 0.25f, 0.34f), FName("Paintable")))
		{
			Shard->Tags.Add(FName("PublicObjective_Target"));
			UCanergyPaintableSurfaceComponent* Surface = NewObject<UCanergyPaintableSurfaceComponent>(Shard);
			Shard->AddInstanceComponent(Surface);
			Surface->RegisterComponent();
			PublicObjectiveActors.Add(Shard);
		}
	}
}

void AShooterGameMode::ClearPublicObjective()
{
	for (const TWeakObjectPtr<AStaticMeshActor>& WeakActor : PublicObjectiveActors)
	{
		if (AStaticMeshActor* Actor = WeakActor.Get()) Actor->Destroy();
	}
	PublicObjectiveActors.Reset();
	PublicObjectivePaintedActors.Reset();
	PublicObjectiveContributors.Reset();
	bObjectiveSpawnedThisPhase = false;
}

void AShooterGameMode::SpawnCarnivalBalloons()
{
	UStaticMesh* BalloonMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!BalloonMesh) return;

	FRandomStream RandomStream(FMath::RoundToInt(MatchRuntimeState.ElapsedSeconds * 1000.0f) + 73);
	for (int32 Index = 0; Index < 8; ++Index)
	{
		const FVector Offset = MatchForwardDirection * RandomStream.FRandRange(500.0f, 2100.0f)
			+ MatchRightDirection * RandomStream.FRandRange(-900.0f, 900.0f);
		const FVector Location = MatchAnchorLocation + Offset + FVector::UpVector * RandomStream.FRandRange(650.0f, 1200.0f);
		const FLinearColor Color = FLinearColor::MakeRandomColor();
		if (AStaticMeshActor* Balloon = SpawnColoredBlock(GetWorld(), BalloonMesh, BaseMaterial,
			Location, FRotator::ZeroRotator, FVector(0.48f), Color, FName("Paintable")))
		{
			Balloon->Tags.Add(FName("CarnivalCollectible"));
			Balloon->SetReplicateMovement(true);
			if (UStaticMeshComponent* Mesh = Balloon->GetStaticMeshComponent())
			{
				Mesh->SetSimulatePhysics(true);
				Mesh->SetEnableGravity(true);
				Mesh->SetMassOverrideInKg(NAME_None, 0.35f, true);
			}
			Balloon->SetLifeSpan(32.0f);
			CarnivalCollectibles.Add(Balloon);
		}
	}
}

void AShooterGameMode::ApplyCarnivalEvent(ECanergyCarnivalEventKind EventKind)
{
	TArray<AActor*> Characters;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShooterCharacter::StaticClass(), Characters);
	const bool bEventEnding = EventKind == ECanergyCarnivalEventKind::None && bCarnivalEventWasActive;
	if (EventKind != ECanergyCarnivalEventKind::None) bCarnivalEventWasActive = true;
	else bCarnivalEventWasActive = false;
	const float GravityMultiplier = EventKind == ECanergyCarnivalEventKind::LowGravity ? 0.48f : 1.0f;
	const float FireRateMultiplier = EventKind == ECanergyCarnivalEventKind::ColorStorm ? 0.62f : 1.0f;
	for (AActor* Actor : Characters)
	{
		if (AShooterCharacter* Character = Cast<AShooterCharacter>(Actor))
		{
			Character->SetCarnivalGravityMultiplier(GravityMultiplier);
			if (bEventEnding)
			{
				if (UCanergyJoyScoreComponent* Joy = Character->FindComponentByClass<UCanergyJoyScoreComponent>())
				{
					Joy->AwardJoyAuthorityOnly(ECanergyJoyAction::CarnivalParticipation);
				}
			}
			if (UCanergyToyWeaponControllerComponent* WeaponController =
				Character->FindComponentByClass<UCanergyToyWeaponControllerComponent>())
			{
				WeaponController->SetFireRateMultiplier(FireRateMultiplier);
			}
		}
	}

	if (EventKind == ECanergyCarnivalEventKind::BalloonRain)
	{
		SpawnCarnivalBalloons();
	}
	else if (EventKind == ECanergyCarnivalEventKind::None)
	{
		for (const TWeakObjectPtr<AStaticMeshActor>& WeakBalloon : CarnivalCollectibles)
		{
			if (AStaticMeshActor* Balloon = WeakBalloon.Get()) Balloon->Destroy();
		}
		CarnivalCollectibles.Reset();
	}

	if (EventKind != ECanergyCarnivalEventKind::None)
	{
		static const TCHAR* EventNames[] = { TEXT("低重力跳跳场"), TEXT("彩能狂喷"), TEXT("气球雨") };
		const int32 EventIndex = EventKind == ECanergyCarnivalEventKind::LowGravity ? 0
			: EventKind == ECanergyCarnivalEventKind::ColorStorm ? 1 : 2;
		ShowPlayerFeedback(FText::FromString(FString::Printf(TEXT("全场狂欢 · %s · 持续约25秒"), EventNames[EventIndex])), 3.0f);
	}
}

void AShooterGameMode::RegisterPublicObjectiveHit(AActor* Target, AActor* Contributor)
{
	if (!HasAuthority() || !Target || !Contributor
		|| MatchRuntimeState.ActiveObjective == ECanergyPublicObjectiveKind::None
		|| !Target->ActorHasTag(FName("PublicObjective_Target"))) return;

	const UCanergyPaintableSurfaceComponent* Surface = Target->FindComponentByClass<UCanergyPaintableSurfaceComponent>();
	if (!Surface || !Surface->IsPainted()) return;
	PublicObjectivePaintedActors.Add(Target);
	PublicObjectiveContributors.Add(Contributor);
	if (PublicObjectivePaintedActors.Num() < PublicObjectiveActors.Num()) return;

	if (!CainengGameRules::CompletePublicObjective(MatchRuntimeState)) return;
	for (const TWeakObjectPtr<AActor>& WeakContributor : PublicObjectiveContributors)
	{
		if (ACharacter* Character = Cast<ACharacter>(WeakContributor.Get()))
		{
			if (UCanergyJoyScoreComponent* Joy = Character->FindComponentByClass<UCanergyJoyScoreComponent>())
			{
				Joy->AwardJoyAuthorityOnly(ECanergyJoyAction::PublicObjective);
			}
		}
	}
	ClearPublicObjective();
	if (RelayObjectiveWidget)
	{
		RelayObjectiveWidget->SetGameplayStatus(FText::FromString(TEXT(
			"自由游玩 · 喷绘地形，尝试弹跳/滑翔路线\n公共目标、狂欢事件可参加，也可自由探索\nWASD 移动 · 空格跳跃 · Shift 冲刺 · 鼠标观察\n左键喷绘/开火 · R 换彩能 · Q 换玩具\nZ 选技能 · F 发动 · E 武器特殊\nCtrl 潜行/上墙 · 墙上空格蹬跳")));
	}
	ShowPlayerFeedback(FText::FromString(TEXT("公共目标完成！参与者获得欢乐值。")), 3.0f);
	UpdateReplicatedMatchState();
}

void AShooterGameMode::AddCarnivalMeter(float Amount)
{
	if (!HasAuthority() || Amount <= 0.0f) return;
	CainengGameRules::AddCarnivalMeter(MatchRuntimeState, Amount, MatchRulesSettings);
	UpdateReplicatedMatchState();
}

void AShooterGameMode::RequestNewRound()
{
	if (!HasAuthority() || MatchRuntimeState.Phase != ECanergyMatchPhase::Finished) return;
	UE_LOG(LogTemp, Display, TEXT("CANERGY_RESULTS restart requested"));
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

void AShooterGameMode::SpawnColorTraversalTrack(const AActor* PlayerStart)
{
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!PlayerStart || !CubeMesh) return;

	static const FName SurfaceTags[] = { FName("Surface_Cyan"), FName("Surface_Orange"), FName("Surface_Magenta") };
	static const FLinearColor SurfaceColors[] = { FLinearColor(0.02f, 0.78f, 1.0f), FLinearColor(1.0f, 0.43f, 0.04f), FLinearColor(1.0f, 0.08f, 0.56f) };
	static const ECanergySurfaceKind SurfaceKinds[] = { ECanergySurfaceKind::Liquid, ECanergySurfaceKind::Bounce, ECanergySurfaceKind::Float };
	const FVector Forward = PlayerStart->GetActorForwardVector();
	const FVector StartLocation = PlayerStart->GetActorLocation();
	const FRotator PadRotation(0.0f, PlayerStart->GetActorRotation().Yaw, 0.0f);

	for (int32 Index = 0; Index < 3; ++Index)
	{
		FVector PadLocation = StartLocation + Forward * (470.0f + Index * 300.0f);
		PadLocation.Z = FindGroundZ(GetWorld(), PadLocation, PlayerStart) + 4.0f;
		if (AStaticMeshActor* Pad = SpawnColoredBlock(GetWorld(), CubeMesh, BaseMaterial, PadLocation, PadRotation,
			FVector(2.8f, 2.2f, 0.08f), SurfaceColors[Index], FName("Paintable")))
		{
			Pad->Tags.Add(SurfaceTags[Index]);
			UCanergyPaintableSurfaceComponent* SurfaceState = NewObject<UCanergyPaintableSurfaceComponent>(Pad);
			Pad->AddInstanceComponent(SurfaceState);
			SurfaceState->RegisterComponent();
			SurfaceState->ApplySurfaceKindAuthorityOnly(SurfaceKinds[Index]);
		}
	}
}

void AShooterGameMode::SpawnRelayObjective(const AActor* PlayerStart)
{
	UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!PlayerStart || !SphereMesh || !CubeMesh) return;

	static const FName RelayTags[] = { FName("Relay_Cyan"), FName("Relay_Orange"), FName("Relay_Magenta") };
	static const FLinearColor RelayColors[] = { FLinearColor(0.02f, 0.78f, 1.0f), FLinearColor(1.0f, 0.43f, 0.04f), FLinearColor(1.0f, 0.08f, 0.56f) };
	const FVector Forward = PlayerStart->GetActorForwardVector();
	const FVector Right = PlayerStart->GetActorRightVector();
	const FVector StartLocation = PlayerStart->GetActorLocation();
	RelayForwardDirection = Forward.GetSafeNormal2D();
	RelayRightDirection = Right.GetSafeNormal2D();
	const FRotator Facing(0.0f, PlayerStart->GetActorRotation().Yaw, 0.0f);

	for (int32 Index = 0; Index < 3; ++Index)
	{
		FVector TargetLocation = StartLocation + Forward * 1450.0f + Right * ((Index - 1) * 150.0f);
		TargetLocation.Z = FindGroundZ(GetWorld(), TargetLocation, PlayerStart) + 175.0f;
		if (AStaticMeshActor* Target = SpawnColoredBlock(GetWorld(), SphereMesh, BaseMaterial, TargetLocation, Facing,
			FVector(0.72f), RelayColors[Index], RelayTags[Index]))
		{
			Target->Tags.Add(FName("Relay_Target"));
			RelayTargets.Add(Target);
		}
	}

	FVector GateLocation = StartLocation + Forward * 1900.0f;
	GateLocation.Z = FindGroundZ(GetWorld(), GateLocation, PlayerStart) + 165.0f;
	RelayGate = SpawnColoredBlock(GetWorld(), CubeMesh, BaseMaterial, GateLocation, Facing,
		FVector(0.35f, 2.8f, 3.3f), FLinearColor(0.16f, 0.20f, 0.30f), FName("Relay_Gate"));

	FVector HurdleLocation = StartLocation + Forward * 2250.0f;
	HurdleLocation.Z = FindGroundZ(GetWorld(), HurdleLocation, PlayerStart) + 55.0f;
	RelayHurdleLocation = HurdleLocation;
	SpawnColoredBlock(GetWorld(), CubeMesh, BaseMaterial, HurdleLocation, Facing,
		FVector(1.2f, 2.8f, 1.1f), FLinearColor(1.0f, 0.55f, 0.06f), FName("Relay_JumpHurdle"));

	FVector ExitLocation = StartLocation + Forward * 2650.0f;
	ExitLocation.Z = FindGroundZ(GetWorld(), ExitLocation, PlayerStart) + 5.0f;
	SpawnColoredBlock(GetWorld(), CubeMesh, BaseMaterial, ExitLocation, Facing,
		FVector(1.8f, 2.8f, 0.1f), FLinearColor(0.10f, 0.95f, 0.45f), FName("Relay_Exit"));
	RelayExitLocation = ExitLocation + FVector::UpVector * 90.0f;
}

void AShooterGameMode::RegisterRelayHit(AActor* HitActor, int32 PaintMode)
{
	if (!HitActor || RelayProgress >= 3) return;
	FText FeedbackMessage;
	static const FName RequiredTags[] = { FName("Relay_Cyan"), FName("Relay_Orange"), FName("Relay_Magenta") };
	static const TCHAR* RelayColorNames[] = { TEXT("青色"), TEXT("橙色"), TEXT("洋红色") };
	int32 HitIndex = -1;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (HitActor->ActorHasTag(RequiredTags[Index]))
		{
			HitIndex = Index;
			break;
		}
	}
	if (HitIndex < 0) return;
	if (PaintMode != HitIndex)
	{
		ShowPlayerFeedback(FText::FromString(FString::Printf(TEXT("颜色不匹配 · 切换为%s喷绘后再命中"), RelayColorNames[HitIndex])));
		return;
	}

	const FRelayHitOutcome Outcome = CainengGameRules::ResolveRelayHit(RelayProgress, HitIndex);
	if (Outcome.Resolution == ERelayHitResolution::Invalid) return;
	if (Outcome.Resolution == ERelayHitResolution::AlreadyCalibrated)
	{
		ShowPlayerFeedback(FText::FromString(TEXT("该中继已校准 · 继续寻找下一个颜色")));
		return;
	}

	RelayProgress = Outcome.NewProgress;
	if (Outcome.Resolution == ERelayHitResolution::Advanced)
	{
		HitActor->SetActorHiddenInGame(true);
		HitActor->SetActorEnableCollision(false);
		FeedbackMessage = FText::FromString(FString::Printf(TEXT("中继校准 %d/3"), RelayProgress));
	}
	else
	{
		for (AStaticMeshActor* Target : RelayTargets)
		{
			if (Target)
			{
				Target->SetActorHiddenInGame(false);
				Target->SetActorEnableCollision(true);
				if (RelayProgress == 1 && Target->ActorHasTag(RequiredTags[0]))
				{
					Target->SetActorHiddenInGame(true);
					Target->SetActorEnableCollision(false);
				}
			}
		}
		const TCHAR* ExpectedColor = RelayProgress == 1 ? TEXT("橙色") : TEXT("青色");
		FeedbackMessage = FText::FromString(FString::Printf(TEXT("顺序错误 · 请先校准%s中继（当前 %d/3）"), ExpectedColor, RelayProgress));
	}

	if (RelayProgress == 3)
	{
		const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		PreviousRelayHurdleSide = PlayerPawn
			? FVector::DotProduct(PlayerPawn->GetActorLocation() - RelayHurdleLocation, RelayForwardDirection)
			: -1.0f;
		bJumpedCalibrationHurdle = false;
		if (RelayGate)
		{
			RelayGate->SetActorHiddenInGame(true);
			RelayGate->SetActorEnableCollision(false);
		}
		FeedbackMessage = FText::FromString(TEXT("中继门已开启！跳过校准梁，抵达绿色出口。"));
	}
	UpdateRelayObjectiveWidget();
	if (!FeedbackMessage.IsEmpty()) ShowPlayerFeedback(FeedbackMessage, RelayProgress == 3 ? 3.0f : 1.8f);
}

AActor* AShooterGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// build the current player tag
	FName PlayerTag = FName(*FString::Printf(TEXT("Player%d"), CurrentPlayerStartAssignment));

	// find all player starts with the matching player tag
	TArray<AActor*> PlayerStarts;

	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), PlayerTag, PlayerStarts);

	// increment the player start assignment index
	++CurrentPlayerStartAssignment;

	// if no PlayerStarts were found, default to all PlayerStarts instead
	if (PlayerStarts.IsEmpty())
	{
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	}

	// have we found at least one PlayerStart?
	if (!PlayerStarts.IsEmpty())
	{
		return PlayerStarts[ FMath::RandRange(0, PlayerStarts.Num() - 1) ];
	}

	// no PlayerStarts in the level
	return nullptr;
}

void AShooterGameMode::IncrementTeamScore(uint8 TeamByte)
{
	// retrieve the team score if any
	int32 Score = 0;
	if (int32* FoundScore = TeamScores.Find(TeamByte))
	{
		Score = *FoundScore;
	}

	// increment the score for the given team
	++Score;
	TeamScores.Add(TeamByte, Score);

	// update the UI
	if (ShooterUI)
	{
		ShooterUI->BP_UpdateScore(TeamByte, Score);
	}
}

bool AShooterGameMode::ShouldSpawnEnemyNPCs() const
{
	// Combat bots are opt-in; traversal testing should not be interrupted by the template arena AI.
	return bSpawnEnemyNPCs && NumberOfLocalPlayers < 2;
}
