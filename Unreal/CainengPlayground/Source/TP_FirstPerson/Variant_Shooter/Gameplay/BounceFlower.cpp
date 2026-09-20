#include "BounceFlower.h"
#include "CanergyRuntimeComponents.h"
#include "WallTraversalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

ACanergyBounceFlower::ACanergyBounceFlower()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.05f;
	Placeholder = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Replaceable Bloom Visual"));
	RootComponent = Placeholder;
	Placeholder->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (Mesh.Succeeded()) Placeholder->SetStaticMesh(Mesh.Object);
	if (Material.Succeeded()) Placeholder->SetMaterial(0, Material.Object);
	SetLifeSpan(8.0f);
}

void ACanergyBounceFlower::Configure(float Radius, float LaunchSpeed, float Lifetime)
{
	EffectRadius = FMath::Clamp(Radius, 80.0f, 450.0f);
	UpSpeed = FMath::Clamp(LaunchSpeed, 400.0f, 1400.0f);
	Placeholder->SetRelativeScale3D(FVector(EffectRadius / 50.0f, EffectRadius / 50.0f, 0.12f));
	if (auto* Material = Placeholder->CreateAndSetMaterialInstanceDynamic(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.4f, 0.04f));
		Material->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(1.0f, 0.4f, 0.04f));
	}
	SetLifeSpan(FMath::Clamp(Lifetime, 1.0f, 20.0f));
}

void ACanergyBounceFlower::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority()) return;
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_Pawn);
	Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CanergyBloomLaunch), false, this);
	TArray<FOverlapResult> Hits;
	GetWorld()->OverlapMultiByObjectType(Hits, GetActorLocation(), FQuat::Identity, Objects,
		FCollisionShape::MakeSphere(EffectRadius), Params);
	const float Now = GetWorld()->GetTimeSeconds();
	for (auto It = NextLaunchTime.CreateIterator(); It; ++It)
		if (!It.Key().IsValid() || It.Value() < Now - 1.0f) It.RemoveCurrent();
	TSet<AActor*> Visited;
	for (const FOverlapResult& Hit : Hits)
	{
		AActor* Target = Hit.GetActor();
		if (!Target || Visited.Contains(Target)) continue;
		Visited.Add(Target);
		if (const float* Next = NextLaunchTime.Find(Target); Next && *Next > Now) continue;
		FHitResult Occlusion;
		FCollisionQueryParams VisibilityParams(SCENE_QUERY_STAT(CanergyBloomOcclusion), false, this);
		if (GetWorld()->LineTraceSingleByChannel(Occlusion, GetActorLocation() + FVector::UpVector * 15.0f,
			Target->GetActorLocation(), ECC_Visibility, VisibilityParams) && Occlusion.GetActor() != Target) continue;
		if (auto* Character = Cast<ACharacter>(Target))
		{
			const auto* Bubble = Character->FindComponentByClass<UCanergyBubbleRespawnComponent>();
			if (Bubble && Bubble->IsBubbled()) continue;
			const float FootHeight = Character->GetActorLocation().Z
				- Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - GetActorLocation().Z;
			if (FootHeight < -30.0f || FootHeight > 65.0f) continue;
			if (auto* Wall = Character->FindComponentByClass<UCanergyWallTraversalComponent>()) Wall->Cancel();
			Character->LaunchCharacter(FVector::UpVector * UpSpeed, false, true);
			NextLaunchTime.Add(Target, Now + 0.7f);
		}
		else if (auto* Body = Hit.GetComponent(); Body && Body->IsSimulatingPhysics() && Body->GetMass() <= 100.0f)
		{
			Body->AddImpulse(FVector::UpVector * UpSpeed, NAME_None, true);
			if (Target->ActorHasTag(TEXT("CanergyCarryCore")))
				UE_LOG(LogTemp, Display, TEXT("CANERGY_OBJECTIVE_TOOL bloom launched core speed=%.1f"), UpSpeed);
			NextLaunchTime.Add(Target, Now + 0.7f);
		}
	}
}
