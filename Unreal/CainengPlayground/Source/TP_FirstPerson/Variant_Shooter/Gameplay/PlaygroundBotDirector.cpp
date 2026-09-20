#include "PlaygroundBotDirector.h"
#include "ShooterCharacter.h"
#include "ShooterGameMode.h"
#include "CanergyRuntimeComponents.h"
#include "WeaponSpecialComponent.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ACanergyPlaygroundBotDirector::ACanergyPlaygroundBotDirector()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ACanergyPlaygroundBotDirector::SpawnParticipants()
{
	auto* Player = Cast<AShooterCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));
	if (!Player) return;
	bSpawnAttempted = true;
	bProbe = FParse::Param(FCommandLine::Get(), TEXT("CanergyBotProbe"));
	const FVector Forward = Player->GetActorForwardVector();
	const FVector Right = Player->GetActorRightVector();
	for (int32 Attempt = 0; Attempt < 25 && Bots.Num() < 5; ++Attempt)
	{
		const int32 Index = Bots.Num();
		const FVector Candidate = Player->GetActorLocation() + Forward * (500.0f + (Attempt % 5) * 160.0f)
			+ Right * (((Attempt % 5) - 2) * 230.0f + (Attempt / 5) * 180.0f);
		FHitResult Ground;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(CanergyBotSpawn), false, Player);
		if (!GetWorld()->LineTraceSingleByChannel(Ground, Candidate + FVector::UpVector * 800.0f,
			Candidate - FVector::UpVector * 2200.0f, ECC_Visibility, Params) || Ground.ImpactNormal.Z < 0.7f) continue;
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
		const FVector Location = Ground.ImpactPoint + FVector::UpVector * (Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 5.0f);
		auto* Bot = GetWorld()->SpawnActor<AShooterCharacter>(Player->GetClass(), Location, Player->GetActorRotation(), Spawn);
		if (!Bot) continue;
		Bot->Tags.Add(TEXT("CanergyBot"));
		if (AController* Previous = Bot->GetController()) { Previous->UnPossess(); Previous->Destroy(); }
		auto* Controller = GetWorld()->SpawnActor<AAIController>();
		if (!Controller) { Bot->Destroy(); continue; }
		Controller->Possess(Bot);
		// The first-person template may ship without a third-person body asset.
		if (!Bot->GetMesh()->GetSkeletalMeshAsset())
		{
			auto* Body = NewObject<UStaticMeshComponent>(Bot, TEXT("Replaceable Bot Placeholder"));
			Bot->AddInstanceComponent(Body);
			Body->SetupAttachment(Bot->GetRootComponent());
			Body->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
			Body->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
			Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Body->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.6f));
			Body->RegisterComponent();
			if (auto* Material = Body->CreateAndSetMaterialInstanceDynamic(0))
			{
				const FLinearColor Color = FLinearColor::MakeFromHSV8(static_cast<uint8>(Index * 45), 210, 255);
				Material->SetVectorParameterValue(TEXT("Color"), Color);
				Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
			}
		}
		UE_LOG(LogTemp, Display, TEXT("CANERGY_BOT_BODY index=%d skeletalMesh=%s"), Index,
			*GetNameSafe(Bot->GetMesh()->GetSkeletalMeshAsset()));
		for (int32 WeaponIndex = 0; WeaponIndex < Index; ++WeaponIndex) Bot->DoSwitchWeapon();
		FBotState& State = Bots.AddDefaulted_GetRef();
		State.Pawn = Bot;
		State.LastLocation = Bot->GetActorLocation();
		State.Heading = Player->GetActorRotation().Yaw + Random.FRandRange(-70.0f, 70.0f);
		State.NextThink = Elapsed + 1.0f + Index * 0.3f;
		State.NextShot = Elapsed + 2.0f + Index * 0.4f;
	}
	UE_LOG(LogTemp, Display, TEXT("CANERGY_BOTS spawned=%d requested=5"), Bots.Num());
	if (auto* Mode = GetWorld()->GetAuthGameMode<AShooterGameMode>())
		Mode->ShowPlayerFeedback(FText::FromString(FString::Printf(TEXT("%d 位游乐伙伴已加入 · 试试喷绘、弹射和吸附互动"), Bots.Num())), 5.0f);
}

void ACanergyPlaygroundBotDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority()) return;
	Elapsed += DeltaSeconds;
	if (!bSpawnAttempted)
	{
		if (Elapsed > 1.5f) SpawnParticipants();
		return;
	}
	const auto* Match = GetWorld()->GetGameState<ACanergyMatchGameState>();
	// Carry slice owns objective steering; keep this actor responsible for spawning only.
	if (FParse::Param(FCommandLine::Get(), TEXT("CanergyCarryPrototype"))) return;
	const bool bFinished = Match && Match->GetCanergyMatchState().Phase == ECanergyMatchPhase::Finished;
	for (int32 Index = 0; Index < Bots.Num(); ++Index)
	{
		FBotState& State = Bots[Index];
		AShooterCharacter* Bot = State.Pawn.Get();
		if (!Bot || !Bot->GetController()) continue;
		const float StepDistance = FVector::Dist2D(State.LastLocation, Bot->GetActorLocation());
		if (StepDistance < 300.0f) State.Distance += StepDistance; // Exclude respawn teleports.
		State.LastLocation = Bot->GetActorLocation();
		if (bFinished || Bot->IsDead() || Bot->GetCanergyBubbleRespawn()->IsBubbled())
		{
			Bot->DoStopFiring(); Bot->SetLiquidDiveHeld(false); continue;
		}
		if (Elapsed >= State.StopShot) Bot->DoStopFiring();
		Bot->DoJumpEnd();
		if (Elapsed >= State.NextThink)
		{
			State.Heading += Random.FRandRange(-95.0f, 95.0f);
			State.NextThink = Elapsed + Random.FRandRange(1.5f, 3.5f);
		}
		FRotator View(-25.0f, State.Heading, 0.0f);
		if (Elapsed >= State.NextShot)
		{
			// Alternate exploration shots with nearby visible player/Bot interactions.
			AShooterCharacter* Target = nullptr;
			float Nearest = 1800.0f;
			TArray<AActor*> Candidates;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShooterCharacter::StaticClass(), Candidates);
			for (AActor* Candidate : Candidates)
			{
				auto* Other = Cast<AShooterCharacter>(Candidate);
				if (!Other || Other == Bot || Other->GetCanergyBubbleRespawn()->IsBubbled()) continue;
				const float Distance = FVector::Dist(Bot->GetActorLocation(), Other->GetActorLocation());
				if (Distance >= Nearest) continue;
				FHitResult Sight;
				FCollisionQueryParams Query(SCENE_QUERY_STAT(CanergyBotSight), false, Bot);
				if (!GetWorld()->LineTraceSingleByChannel(Sight, Bot->GetPawnViewLocation(), Other->GetActorLocation(), ECC_Visibility, Query)
					|| Sight.GetActor() == Other) { Target = Other; Nearest = Distance; }
			}
			if (Target && Index != 0) View = (Target->GetActorLocation() - Bot->GetPawnViewLocation()).Rotation();
			Bot->GetController()->SetControlRotation(View);
			Bot->SetActorRotation(FRotator(0.0f, View.Yaw, 0.0f));
			Bot->DoStartFiring();
			++State.FireCommands;
			State.StopShot = Elapsed + 0.35f;
			State.NextShot = Elapsed + 2.4f + Index * 0.15f;
			if (Index == 0)
				if (auto* Special = Bot->FindComponentByClass<UCanergyWeaponSpecialComponent>()) Special->ActivateSpecial();
		}
		else if (Elapsed >= State.StopShot)
		{
			Bot->GetController()->SetControlRotation(View);
			Bot->SetActorRotation(FRotator(0.0f, State.Heading, 0.0f));
		}
		FHitResult Obstacle;
		FCollisionQueryParams Probe(SCENE_QUERY_STAT(CanergyBotObstacle), false, Bot);
		if (GetWorld()->LineTraceSingleByChannel(Obstacle, Bot->GetActorLocation(),
			Bot->GetActorLocation() + Bot->GetActorForwardVector() * 140.0f, ECC_Visibility, Probe))
		{
			if (Elapsed >= State.NextJump) { Bot->DoJumpStart(); State.NextJump = Elapsed + 1.5f; }
			State.Heading += 90.0f * DeltaSeconds;
		}
		Bot->SetLiquidDiveHeld(Index % 2 == 0 && FMath::Fmod(Elapsed, 6.0f) < 3.0f);
		Bot->DoMove(0.0f, 0.65f);
	}
	if (bProbe && !Bots.IsEmpty() && Bots[0].Pawn.IsValid())
	{
		auto* Bot = Bots[0].Pawn.Get();
		if (Elapsed > 10.0f && !bProbeDamageSent)
		{
			bProbeDamageSent = true;
			Bot->TakeDamage(100000.0f, FDamageEvent(), nullptr, this);
		}
		if (bProbeDamageSent && Elapsed > 13.5f && !Bot->IsDead() && !Bot->GetCanergyBubbleRespawn()->IsBubbled()) bProbeReturned = true;
		if (Elapsed > 35.0f && !bReported)
		{
			bReported = true;
			bool bPass = Bots.Num() == 5 && bProbeReturned;
			for (int32 Index = 0; Index < Bots.Num(); ++Index)
			{
				const FBotState& State = Bots[Index];
				bPass &= State.Pawn.IsValid() && State.Distance > 500.0f && State.FireCommands >= 3;
				UE_LOG(LogTemp, Display, TEXT("CANERGY_BOT index=%d alive=%d distance=%.1f fireCommands=%d"),
					Index, State.Pawn.IsValid(), State.Distance, State.FireCommands);
			}
			UE_LOG(LogTemp, Display, TEXT("CANERGY_BOTS %s participants=%d bubbleReturn=%d"), bPass ? TEXT("PASS") : TEXT("FAIL"), Bots.Num() + 1, bProbeReturned);
		}
	}
}
