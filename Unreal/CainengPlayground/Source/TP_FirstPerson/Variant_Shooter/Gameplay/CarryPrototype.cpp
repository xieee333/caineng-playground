#include "CarryPrototype.h"
#include "ShooterCharacter.h"
#include "ShooterGameMode.h"
#include "CanergyRuntimeComponents.h"
#include "BounceFlower.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"

ACanergyCarryPrototype::ACanergyCarryPrototype()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	bNetworkProbe = FParse::Param(FCommandLine::Get(), TEXT("CanergyCarryNetworkProbe"));
	bBotMatch = FParse::Param(FCommandLine::Get(), TEXT("CanergyCarryBotMatch"));
	InitializeAfter = bNetworkProbe ? 8.0f : 2.0f;
}

AStaticMeshActor* ACanergyCarryPrototype::MakeShape(const TCHAR* Asset, FVector Position, FVector Scale, FLinearColor Color)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Position, FRotator::ZeroRotator, Params);
	if (!Actor) return nullptr;
	Actor->SetReplicates(true);
	auto* Mesh = Actor->GetStaticMeshComponent();
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Asset));
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Actor->SetActorScale3D(Scale);
	Mesh->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
	if (auto* Material = Mesh->CreateAndSetMaterialInstanceDynamic(0))
	{
		TArray<FMaterialParameterInfo> Infos;
		TArray<FGuid> Ids;
		Material->GetAllVectorParameterInfo(Infos, Ids);
		for (const auto& Info : Infos)
		{
			Material->SetVectorParameterValueByInfo(Info, Color);
			UE_LOG(LogTemp, Display, TEXT("CANERGY_CARRY material parameter=%s"), *Info.Name.ToString());
		}
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
	}
	return Actor;
}

void ACanergyCarryPrototype::InitializeArena()
{
	auto* Human = Cast<AShooterCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (!Human) return;
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShooterCharacter::StaticClass(), Found);
	if (Found.Num() < 6) return;
	Players.Add(Human);
	for (auto* Actor : Found)
		if (Actor != Human && Cast<AShooterCharacter>(Actor)->IsPlayerControlled() && Players.Num()<6)
			Players.Add(Cast<AShooterCharacter>(Actor));
	for (auto* Actor : Found)
		if (Actor != Human && Actor->ActorHasTag(TEXT("CanergyBot")) && Players.Num()<6)
			Players.Add(Cast<AShooterCharacter>(Actor));
	if (Players.Num() != 6) { Players.Reset(); return; }
	ProtectedUntil.Init(0.0f, Players.Num());
	PreviousBubbled.Init(false, Players.Num());
	BotFlankReached.Init(false, Players.Num());
	Arena = Human->GetActorLocation() + FVector(0, 0, 3500);
	const TCHAR* Cube = TEXT("/Engine/BasicShapes/Cube.Cube");
	// 54 x 40 m combat field: central fast lane, two readable side routes and room for 3v3 tool combos.
	MakeShape(Cube, Arena - FVector(0,0,30), FVector(54,40,0.6), FLinearColor(0.12f,0.16f,0.21f));
	for (int32 Sign : {-1, 1})
	{
		MakeShape(Cube, Arena + FVector(Sign*ArenaHalfLength,0,140), FVector(0.3f,40,2.8f), FLinearColor(0.2f,0.25f,0.3f));
		MakeShape(Cube, Arena + FVector(0,Sign*ArenaHalfWidth,140), FVector(54,0.3f,2.8f), FLinearColor(0.2f,0.25f,0.3f));
		// Offset cover breaks cross-map sightlines without cluttering the objective circle.
		MakeShape(Cube, Arena + FVector(Sign*720,Sign*560,90), FVector(3.2f,1.8f,1.8f), FLinearColor(0.3f,0.34f,0.4f));
	}
	// Northern raised route: low steps keep it reachable without a bespoke traversal animation.
	MakeShape(Cube, Arena + FVector(-900,1080,15), FVector(5.5f,4.2f,0.3f), FLinearColor(0.18f,0.42f,0.52f));
	MakeShape(Cube, Arena + FVector(0,1080,30), FVector(12.5f,4.2f,0.6f), FLinearColor(0.18f,0.46f,0.56f));
	MakeShape(Cube, Arena + FVector(900,1080,15), FVector(5.5f,4.2f,0.3f), FLinearColor(0.18f,0.42f,0.52f));
	// Southern route has a phase-only shortcut, but remains passable around both ends.
	MakeShape(Cube, Arena + FVector(-900,-1080,65), FVector(5.0f,2.2f,1.3f), FLinearColor(0.28f,0.31f,0.38f));
	MakeShape(Cube, Arena + FVector(900,-1080,65), FVector(5.0f,2.2f,1.3f), FLinearColor(0.28f,0.31f,0.38f));
	auto* PhaseWall = MakeShape(Cube, Arena + FVector(0,-1080,130), FVector(0.6f,3.0f,2.6f), FLinearColor(0.4f,0.08f,0.72f));
	if (PhaseWall) PhaseWall->Tags.Add(TEXT("CanergyPhaseWall"));
	for (int32 T=0; T<2; ++T)
	{
		Goals[T] = Arena + FVector(T == 0 ? -GoalOffset : GoalOffset, 0, 12);
		auto* Goal = MakeShape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), Goals[T], FVector(4.4,4.4,0.18),
			T == 0 ? FLinearColor(0.02f,0.75f,1) : FLinearColor(1,0.3f,0.04f));
		if (Goal) Goal->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	for (int32 I=0; I<Players.Num(); ++I)
	{
		auto* P = Players[I].Get();
		P->SetTeam(Team(I));
		const bool bRemoteHumanProbe = bNetworkProbe && I > 0 && P->IsPlayerControlled();
		const bool bRemotePhaseProbe = bRemoteHumanProbe && FParse::Param(FCommandLine::Get(), TEXT("CanergyPhaseNetworkProbe"));
		const bool bPhaseProbeHuman = I == 0 && FParse::Param(FCommandLine::Get(), TEXT("CanergyPhaseProbe"));
		const FVector Spawn = bPhaseProbeHuman || bRemotePhaseProbe
			? Arena + FVector(-250,-1080,120)
			: bRemoteHumanProbe
			? Arena + FVector(-130, 0, 120)
			: Arena + FVector(Team(I)==0 ? -SpawnOffset : SpawnOffset, (I/2-1)*320, 120);
		P->SetActorLocation(Spawn, false, nullptr, ETeleportType::TeleportPhysics);
		P->GetCharacterMovement()->StopMovementImmediately();
		const FRotator Facing(0, (bPhaseProbeHuman || bRemotePhaseProbe) ? 0 : Team(I)==0 ? 0 : 180, 0);
		P->SetActorRotation(Facing);
		if (P->GetController()) P->GetController()->SetControlRotation(Facing);
		if (I>0)
		{
			const int32 BotRole = I / 2;
			static const TCHAR* RoleNames[] = { TEXT("搬"), TEXT("截"), TEXT("援") };
			auto* Label=NewObject<UTextRenderComponent>(P);
			P->AddInstanceComponent(Label);
			Label->SetupAttachment(P->GetRootComponent());
			Label->SetRelativeLocation(FVector(0,0,145));
			Label->SetText(FText::FromString(FString::Printf(TEXT("%s-%s"), Team(I)==0 ? TEXT("A") : TEXT("B"), RoleNames[FMath::Clamp(BotRole,0,2)])));
			Label->SetTextRenderColor(Team(I)==0 ? FColor::Cyan : FColor::Orange);
			Label->SetWorldSize(55);
			Label->SetHorizontalAlignment(EHTA_Center);
			Label->RegisterComponent();
			TeamLabels.Add(Label);
		}
	}
	Core = MakeShape(TEXT("/Engine/BasicShapes/Sphere.Sphere"), Arena + FVector(0,0,100), FVector(0.55), FLinearColor(1,0.78f,0.12f));
	if (!Core) return;
	Core->Tags.Add(TEXT("CanergyCarryCore"));
	// The objective is a real lightweight physics prop.  Explicitly classify it as
	// PhysicsBody so shared toy tools (Bloom, suction, inflation) discover it through
	// the same object query as every other movable prop.
	Core->GetStaticMeshComponent()->SetCollisionObjectType(ECC_PhysicsBody);
	Core->GetStaticMeshComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Core->GetStaticMeshComponent()->SetSimulatePhysics(true);
	Core->GetStaticMeshComponent()->SetMassOverrideInKg(NAME_None, 12.0f);
	Core->GetStaticMeshComponent()->SetLinearDamping(0.7f);
	bProbe = FParse::Param(FCommandLine::Get(), TEXT("CanergyCarryProbe"));
	bPractice = FParse::Param(FCommandLine::Get(), TEXT("CanergyCarryPractice"));
	bObjectiveToolProbe = FParse::Param(FCommandLine::Get(), TEXT("CanergyCarryObjectiveToolProbe"));
	if (bPractice) Human->SetActorLocation(Arena+FVector(-130,0,120),false,nullptr,ETeleportType::TeleportPhysics);
	bReady = true;
	NextProbe = Age + 1.0f;
	int32 HumanCount = 0;
	for (const auto& Player : Players) if (Player.IsValid() && Player->IsPlayerControlled()) ++HumanCount;
	UE_LOG(LogTemp, Display, TEXT("CANERGY_CARRY ready players=%d humans=%d arena=%s size=54x40m probe=%d networkProbe=%d"),
		Players.Num(), HumanCount, *Arena.ToString(), bProbe, bNetworkProbe);
	UpdateHud();
}

int32 ACanergyCarryPrototype::FindPlayer(const AShooterCharacter* Character) const
{
	for (int32 I=0; I<Players.Num(); ++I) if (Players[I].Get()==Character) return I;
	return INDEX_NONE;
}

bool ACanergyCarryPrototype::CanPlayerContest(int32 Index) const
{
	if (!Players.IsValidIndex(Index) || !Players[Index].IsValid()) return false;
	const auto* Player = Players[Index].Get();
	const float Protection = ProtectedUntil.IsValidIndex(Index) ? ProtectedUntil[Index] - Age : 0.0f;
	return CainengGameRules::CanContestCore(Player->IsDead(), Player->GetCanergyBubbleRespawn()->IsBubbled(), Protection);
}

void ACanergyCarryPrototype::Interact(AShooterCharacter* Character)
{
	if (!HasAuthority() || !bReady || !Character || Rules.bFinished) return;
	const int32 Index = FindPlayer(Character);
	if (Index == INDEX_NONE || !CanPlayerContest(Index)) return;
	if (Rules.Carrier == Index) { Dislodge(Character); return; }
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(CarryPickup), false, Character);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Character->GetPawnViewLocation(), Core->GetActorLocation(), ECC_Visibility, Query);
	const bool bReach = FVector::Dist(Character->GetActorLocation(), Core->GetActorLocation()) <= 230.0f && (!bHit || Hit.GetActor()==Core);
	if (CainengGameRules::TryTakeCore(Rules, Index, Team(Index), true, bReach))
	{
		Core->GetStaticMeshComponent()->SetSimulatePhysics(false);
		Core->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LooseClock = 0;
		UE_LOG(LogTemp, Display, TEXT("CANERGY_CARRY take player=%d team=%d"), Index, Team(Index));
		UpdateHud();
	}
}

void ACanergyCarryPrototype::Dislodge(AShooterCharacter* Character)
{
	if (!HasAuthority() || !bReady || !Character) return;
	if (!CainengGameRules::DropCore(Rules, FindPlayer(Character))) return;
	auto* Mesh = Core->GetStaticMeshComponent();
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetSimulatePhysics(true);
	Mesh->SetPhysicsLinearVelocity(Character->GetControlRotation().Vector() * 1200.0f + FVector(0,0,220));
	LooseClock = 0;
	UE_LOG(LogTemp, Display, TEXT("CANERGY_CARRY drop player=%d"), FindPlayer(Character));
	UpdateHud();
}

void ACanergyCarryPrototype::HandleWeaponHit(AShooterCharacter* Target, AShooterCharacter* Attacker)
{
	const int32 TargetIndex = FindPlayer(Target), AttackerIndex = FindPlayer(Attacker);
	if (TargetIndex >= 0 && AttackerIndex >= 0 && CanPlayerContest(TargetIndex)
		&& Team(TargetIndex) != Team(AttackerIndex)) Dislodge(Target);
}

void ACanergyCarryPrototype::ResetCore()
{
	Rules.Carrier = Rules.CarrierTeam = INDEX_NONE;
	Rules.PickupLock = 2.0f;
	auto* Mesh=Core->GetStaticMeshComponent();
	Mesh->SetSimulatePhysics(false);
	Core->SetActorLocation(Arena+FVector(0,0,100), false, nullptr, ETeleportType::TeleportPhysics);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetSimulatePhysics(true);
	Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	LooseClock=0;
	for (int32 I=0; I<BotFlankReached.Num(); ++I) BotFlankReached[I]=false;
}

void ACanergyCarryPrototype::UpdateHud()
{
	auto* Mode=GetWorld()->GetAuthGameMode<AShooterGameMode>();
	if (!Mode) return;
	FString Status=FString::Printf(TEXT("核心争夺 · 青队 %d : %d 橙队 · 剩余 %d 秒\n你是青队 · 搬回青色圆台 · 先得3分或时间结束\n"), Rules.Scores[0],Rules.Scores[1], FMath::CeilToInt(Rules.Remaining));
	if (Rules.bFinished)
	{
		Status += Rules.Scores[0]==Rules.Scores[1] ? TEXT("平局") : Rules.Scores[0]>Rules.Scores[1] ? TEXT("青队获胜") : TEXT("橙队获胜");
		Status += TEXT(" · 回车再开一局");
	}
	else
	{
		Status += Rules.Carrier==0 ? TEXT("你携带核心 · 回青色圆台得分\n") : Rules.Carrier<0 ? TEXT("金色核心可争夺 · 靠近后按 G 拿取\n") : FString::Printf(TEXT("%s队持球 · 可以拦截\n"), Rules.CarrierTeam==0 ? TEXT("青") : TEXT("橙"));
		Status += TEXT("G 拿取 / 向准星抛出 · 受击掉球\nWASD 移动 · 鼠标观察 · 空格跳跃\n左键喷绘 · Q 换枪 · E 盛放可弹飞玩家与松散核心\nF 技能 · X 潜相裂光薄墙");
	}
	Mode->SetPrototypeStatus(FText::FromString(Status));
}

void ACanergyCarryPrototype::DriveBots(float DeltaSeconds)
{
	for(int32 I=bBotMatch ? 0 : 1; I<Players.Num(); ++I)
	{
		auto* P=Players[I].Get();
		if (!P || !P->GetController() || P->IsDead() || P->GetCanergyBubbleRespawn()->IsBubbled()) continue;
		const int32 BotRole=FMath::Clamp(I/2,0,2); // 0 搬运、1 拦截、2 支援；每队组合不同但互补。
		FVector Target=Core->GetActorLocation();
		if (Rules.Carrier==I)
		{
			Target=Goals[Team(I)];
		}
		else if (Rules.Carrier>=0 && Team(Rules.Carrier)!=Team(I) && Players.IsValidIndex(Rules.Carrier) && Players[Rules.Carrier].IsValid())
		{
			Target=Players[Rules.Carrier]->GetActorLocation();
		}
		else if (Rules.Carrier>=0 && Team(Rules.Carrier)==Team(I))
		{
			// Support runs ahead toward the scoring half; interceptor shadows the likely pursuit lane.
			const FVector CarrierLocation=Players[Rules.Carrier].IsValid() ? Players[Rules.Carrier]->GetActorLocation() : Core->GetActorLocation();
			Target=CarrierLocation + FVector(Team(I)==0 ? -520.0f : 520.0f, BotRole==2 ? -420.0f : 420.0f, 0);
		}
		else if (BotRole>0 && BotFlankReached.IsValidIndex(I) && !BotFlankReached[I])
		{
			// Interceptor samples the raised north route; support goes around the phase wall's south end.
			const FVector RoutePoint=Arena+FVector(Team(I)==0 ? -120.0f : 120.0f, BotRole==1 ? 1080.0f : -1520.0f, 120.0f);
			if(FVector::Dist2D(P->GetActorLocation(),RoutePoint)<260.0f)
			{
				BotFlankReached[I]=true;
				UE_LOG(LogTemp, Display, TEXT("CANERGY_CARRY route player=%d role=%s lane=%s"), I,
					BotRole==1 ? TEXT("interceptor") : TEXT("support"), BotRole==1 ? TEXT("north") : TEXT("south"));
			}
			else Target=RoutePoint;
		}
		const FRotator Facing(0,(Target-P->GetActorLocation()).Rotation().Yaw,0);
		P->GetController()->SetControlRotation(Facing);
		P->SetActorRotation(Facing);
		P->SetLiquidDiveHeld(false);
		P->DoMove(0, Rules.Carrier==I ? 0.60f : BotRole==0 ? 0.52f : 0.48f);
		if (Rules.Carrier<0 && FVector::Dist2D(P->GetActorLocation(),Core->GetActorLocation())<250.0f) Interact(P);
		if (Rules.Carrier>=0 && Team(Rules.Carrier)!=Team(I) && FVector::Dist2D(P->GetActorLocation(), Target)<1100)
		{
			if (FMath::Fmod(Age+I*0.4f, 3.0f)<0.25f) P->DoStartFiring(); else P->DoStopFiring();
		}
		else P->DoStopFiring();
	}
}

void ACanergyCarryPrototype::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority()) return;
	Age += DeltaSeconds;
	if (!bReady) { if(Age>InitializeAfter) InitializeArena(); return; }
	if (Rules.bFinished) { SyncReplicatedSnapshot(); if (bProbe) RunProbe(); return; }
	if (bProbe) RunProbe();
	if (bObjectiveToolProbe) RunObjectiveToolProbe();
	CainengGameRules::AdvanceCarryClock(Rules, DeltaSeconds);
	if (Rules.Carrier>=0)
	{
		auto* P=Players[Rules.Carrier].Get();
		if (!P) ResetCore();
		else if (P->IsDead() || P->GetCanergyBubbleRespawn()->IsBubbled()) Dislodge(P);
		else
		{
			const FVector Start = P->GetActorLocation();
			const FVector Desired = Start+P->GetActorForwardVector()*180+P->GetActorRightVector()*90-FVector(0,0,15);
			FHitResult HoldHit;
			FCollisionQueryParams HoldQuery(SCENE_QUERY_STAT(CarryHold), false, P);
			HoldQuery.AddIgnoredActor(Core);
			const bool bBlocked = GetWorld()->SweepSingleByChannel(HoldHit, Start, Desired, FQuat::Identity,
				ECC_Visibility, FCollisionShape::MakeSphere(29), HoldQuery);
			Core->SetActorLocation(bBlocked ? HoldHit.Location : Desired);
			const int32 T=Rules.CarrierTeam;
			const bool bAtGoal=FVector::Dist2D(P->GetActorLocation(),Goals[T])<210 && FMath::Abs(P->GetActorLocation().Z-(Arena.Z+100))<180;
			if (CainengGameRules::TryDeliverCore(Rules,Rules.Carrier,T,bAtGoal))
			{
				UE_LOG(LogTemp,Display,TEXT("CANERGY_CARRY score team=%d cyan=%d orange=%d"),T,Rules.Scores[0],Rules.Scores[1]);
				ResetCore(); UpdateHud();
			}
		}
	}
	else
	{
		LooseClock+=DeltaSeconds;
		const FVector Offset=Core->GetActorLocation()-Arena;
		if(Offset.Z < -200 || FMath::Abs(Offset.X)>ArenaHalfLength+50 || FMath::Abs(Offset.Y)>ArenaHalfWidth+50 || LooseClock>18)
		{
			UE_LOG(LogTemp,Display,TEXT("CANERGY_CARRY recovered loose core")); ResetCore();
		}
	}
	for(int32 I=0; I<Players.Num(); ++I)
	{
		auto* P=Players[I].Get();
		if (!P)
		{
			if (Rules.Carrier==I) { UE_LOG(LogTemp,Warning,TEXT("CANERGY_CARRY carrier lost player=%d"),I); ResetCore(); }
			continue;
		}
		const bool bBubbled=P->GetCanergyBubbleRespawn()->IsBubbled();
		if (PreviousBubbled.IsValidIndex(I) && PreviousBubbled[I] && !bBubbled)
		{
			ProtectedUntil[I]=Age+2.0f;
			UE_LOG(LogTemp,Display,TEXT("CANERGY_CARRY protection player=%d seconds=2.0"),I);
		}
		if (PreviousBubbled.IsValidIndex(I)) PreviousBubbled[I]=bBubbled;
		if(P && !P->IsDead() && P->GetActorLocation().Z<Arena.Z-250)
		{
			Dislodge(P);
			P->SetActorLocation(Arena+FVector(Team(I)==0 ? -SpawnOffset : SpawnOffset,(I/2-1)*320,120),false,nullptr,ETeleportType::TeleportPhysics);
			P->GetCharacterMovement()->StopMovementImmediately();
		}
	}
	if(!bProbe && !bPractice && !bNetworkProbe && !bObjectiveToolProbe && !Rules.bFinished) DriveBots(DeltaSeconds);
	if (Players[0].IsValid())
		for (const auto& Label : TeamLabels)
			if (Label) Label->SetWorldRotation((Players[0]->GetPawnViewLocation()-Label->GetComponentLocation()).Rotation());
	// Diagnostic markers only; final UI and art remain subject to user approval.
	for (int32 T=0; T<2; ++T)
		DrawDebugCylinder(GetWorld(), Goals[T], Goals[T]+FVector(0,0,140), 210, 32,
			T==0 ? FColor::Cyan : FColor::Orange, false, -1, 0, 3);
	if (Rules.Carrier!=0) DrawDebugSphere(GetWorld(), Core->GetActorLocation(), 32, 12, FColor::Yellow, false, -1, 0, 1);
	HudClock+=DeltaSeconds;
	if(HudClock>0.25f || Rules.bFinished) { HudClock=0; UpdateHud(); }
	if(Rules.bFinished)
	{
		for(auto P:Players) if(P.IsValid()) P->DoStopFiring();
		UE_LOG(LogTemp,Display,TEXT("CANERGY_CARRY finished cyan=%d orange=%d remaining=%.1f"),Rules.Scores[0],Rules.Scores[1],Rules.Remaining);
	}
	SyncReplicatedSnapshot();
}

void ACanergyCarryPrototype::SyncReplicatedSnapshot()
{
	if (!HasAuthority()) return;
	const float NewRemaining=static_cast<float>(FMath::CeilToInt(Rules.Remaining));
	AActor* NewCarrier=Players.IsValidIndex(Rules.Carrier) ? Players[Rules.Carrier].Get() : nullptr;
	const bool bChanged=RepScoreA!=Rules.Scores[0] || RepScoreB!=Rules.Scores[1]
		|| RepCarrierTeam!=Rules.CarrierTeam || RepRemaining!=NewRemaining
		|| bRepFinished!=Rules.bFinished || RepCarrier!=NewCarrier;
	RepScoreA=Rules.Scores[0]; RepScoreB=Rules.Scores[1]; RepCarrierTeam=Rules.CarrierTeam;
	RepRemaining=NewRemaining; bRepFinished=Rules.bFinished; RepCarrier=NewCarrier;
	if (bChanged) ForceNetUpdate();
}

void ACanergyCarryPrototype::OnRep_CarrySnapshot()
{
	UE_LOG(LogTemp,Display,TEXT("CANERGY_CARRY_CLIENT score=%d:%d carrierTeam=%d remaining=%.1f finished=%d carrier=%s"),
		RepScoreA,RepScoreB,RepCarrierTeam,RepRemaining,bRepFinished,*GetNameSafe(RepCarrier));
}

void ACanergyCarryPrototype::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ACanergyCarryPrototype,RepScoreA); DOREPLIFETIME(ACanergyCarryPrototype,RepScoreB);
	DOREPLIFETIME(ACanergyCarryPrototype,RepCarrierTeam); DOREPLIFETIME(ACanergyCarryPrototype,RepRemaining);
	DOREPLIFETIME(ACanergyCarryPrototype,bRepFinished); DOREPLIFETIME(ACanergyCarryPrototype,RepCarrier);
}

void ACanergyCarryPrototype::RunProbe()
{
#if !UE_BUILD_SHIPPING
	if(Age<NextProbe || bReported) return;
	NextProbe=Age+2.5f;
	auto* P=Players[0].Get();
	if(!P) return;
	if(ProbeStep==0)
	{
		Interact(P); bProbePass &= Rules.Carrier==INDEX_NONE;
		P->SetActorLocation(Core->GetActorLocation()+FVector(-120,0,65),false,nullptr,ETeleportType::TeleportPhysics);
		Interact(P); bProbePass &= Rules.Carrier==0;
	}
	else if(ProbeStep==1) { Dislodge(P); bProbePass &= Rules.Carrier==INDEX_NONE; }
	else if(ProbeStep==2)
	{
		Core->SetActorLocation(Arena+FVector(0,0,-500),false,nullptr,ETeleportType::TeleportPhysics);
	}
	else if(ProbeStep==3)
	{
		bProbePass &= Core->GetActorLocation().Z>Arena.Z;
		P->SetActorLocation(Core->GetActorLocation()+FVector(-120,0,65),false,nullptr,ETeleportType::TeleportPhysics);
		Interact(P); bProbePass &= Rules.Carrier==0;
		P->SetActorLocation(Goals[0]+FVector(0,0,100),false,nullptr,ETeleportType::TeleportPhysics);
	}
	else if(ProbeStep==4)
	{
		bProbePass &= Rules.Scores[0]==1 && Rules.Carrier==INDEX_NONE;
		P->SetActorLocation(Core->GetActorLocation()+FVector(-120,0,65),false,nullptr,ETeleportType::TeleportPhysics);
		Interact(P); bProbePass &= Rules.Carrier==0;
		HandleWeaponHit(P, Players[2].Get()); bProbePass &= Rules.Carrier==0;
		HandleWeaponHit(P, Players[1].Get()); bProbePass &= Rules.Carrier==INDEX_NONE;
	}
	else if(ProbeStep==5) { ResetCore(); }
	else if(ProbeStep==6)
	{
		P->SetActorLocation(Core->GetActorLocation()+FVector(-120,0,65),false,nullptr,ETeleportType::TeleportPhysics);
		Interact(P); bProbePass &= Rules.Carrier==0;
		P->TakeDamage(100000.0f, FDamageEvent(), nullptr, this);
		bProbePass &= Rules.Carrier==INDEX_NONE;
	}
	else if(ProbeStep==7)
	{
		bProbePass &= P->GetCanergyBubbleRespawn()->IsBubbled();
	}
	else if(ProbeStep==8)
	{
		bProbePass &= !P->GetCanergyBubbleRespawn()->IsBubbled();
		P->SetActorLocation(Core->GetActorLocation()+FVector(-120,0,65),false,nullptr,ETeleportType::TeleportPhysics);
		Interact(P); bProbePass &= Rules.Carrier==INDEX_NONE;
	}
	else if(ProbeStep==9)
	{
		Interact(P); bProbePass &= Rules.Carrier==0;
		Dislodge(P);
		Rules.Remaining=0.05f;
	}
	else if(ProbeStep==10)
	{
		bProbePass &= Rules.bFinished && Rules.Scores[0]==1;
		Interact(P); bProbePass &= Rules.Carrier==INDEX_NONE;
		bReported=true;
		UE_LOG(LogTemp,Display,TEXT("CANERGY_CARRY_PROBE %s reach/pickup/drop/recovery/delivery/enemy-hit/friendly-hit/bubble/protection/timeout score=%d"),bProbePass ? TEXT("PASS") : TEXT("FAIL"),Rules.Scores[0]);
	}
	++ProbeStep;
#endif
}

void ACanergyCarryPrototype::RunObjectiveToolProbe()
{
#if !UE_BUILD_SHIPPING
	if (Age < NextProbe || !Core) return;
	if (ObjectiveToolProbeStep == 0)
	{
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ObjectiveToolProbeFlower = GetWorld()->SpawnActor<ACanergyBounceFlower>(
			ACanergyBounceFlower::StaticClass(), Arena + FVector(0,0,8), FRotator::ZeroRotator, Spawn);
		if (ObjectiveToolProbeFlower.IsValid()) ObjectiveToolProbeFlower->Configure(240.0f, 900.0f, 3.0f);
		Core->GetStaticMeshComponent()->SetPhysicsLinearVelocity(FVector::ZeroVector);
		NextProbe = Age + 0.6f;
		++ObjectiveToolProbeStep;
		return;
	}
	if (ObjectiveToolProbeStep == 1)
	{
		const float VerticalSpeed = Core->GetStaticMeshComponent()->GetPhysicsLinearVelocity().Z;
		const bool bBloomPass = ObjectiveToolProbeFlower.IsValid()
			&& (VerticalSpeed > 250.0f || Core->GetActorLocation().Z > Arena.Z + 150.0f);
		bObjectiveToolProbePass &= bBloomPass;
		UE_LOG(LogTemp, Display, TEXT("CANERGY_OBJECTIVE_TOOL_PROBE bloom=%s verticalSpeed=%.1f height=%.1f"),
			bBloomPass ? TEXT("PASS") : TEXT("FAIL"), VerticalSpeed, Core->GetActorLocation().Z - Arena.Z);
		if (ObjectiveToolProbeFlower.IsValid()) ObjectiveToolProbeFlower->Destroy();
		ResetCore();
		NextProbe = Age + 0.35f;
		++ObjectiveToolProbeStep;
		return;
	}
	if (ObjectiveToolProbeStep == 2 && Players.IsValidIndex(0) && Players[0].IsValid())
	{
		AShooterCharacter* Player = Players[0].Get();
		Player->SetActorLocation(Arena + FVector(-650, 0, 120), false, nullptr, ETeleportType::TeleportPhysics);
		FVector ViewLocation;
		FRotator ViewRotation;
		Player->GetActorEyesViewPoint(ViewLocation, ViewRotation);
		const FRotator Aim = (Core->GetActorLocation() - ViewLocation).Rotation();
		Player->SetActorRotation(FRotator(0, Aim.Yaw, 0));
		if (Player->GetController()) Player->GetController()->SetControlRotation(Aim);
		if (UCanergyToyWeaponControllerComponent* Weapon = Player->FindComponentByClass<UCanergyToyWeaponControllerComponent>())
		{
			Weapon->CycleWeapon(); // Prototype index 1: wide attractor cone.
			ObjectiveToolProbeInitialDistance = FVector::Dist2D(Core->GetActorLocation(), Player->GetActorLocation());
			Weapon->StartFiring();
		}
		NextProbe = Age + 0.7f;
		++ObjectiveToolProbeStep;
		return;
	}
	if (Players.IsValidIndex(0) && Players[0].IsValid())
	{
		AShooterCharacter* Player = Players[0].Get();
		if (UCanergyToyWeaponControllerComponent* Weapon = Player->FindComponentByClass<UCanergyToyWeaponControllerComponent>())
			Weapon->StopFiring();
		const float Distance = FVector::Dist2D(Core->GetActorLocation(), Player->GetActorLocation());
		const bool bAttractorPass = ObjectiveToolProbeInitialDistance - Distance > 30.0f;
		bObjectiveToolProbePass &= bAttractorPass;
		UE_LOG(LogTemp, Display, TEXT("CANERGY_OBJECTIVE_TOOL_PROBE attractor=%s pulled=%.1f"),
			bAttractorPass ? TEXT("PASS") : TEXT("FAIL"), ObjectiveToolProbeInitialDistance - Distance);
	}
	UE_LOG(LogTemp, Display, TEXT("CANERGY_OBJECTIVE_TOOL_PROBE %s bloom+attractor-core"),
		bObjectiveToolProbePass ? TEXT("PASS") : TEXT("FAIL"));
	ResetCore();
	bObjectiveToolProbe = false;
#endif
}
