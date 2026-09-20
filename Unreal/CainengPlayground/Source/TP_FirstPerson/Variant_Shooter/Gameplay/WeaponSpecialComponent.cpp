#include "WeaponSpecialComponent.h"
#include "BounceFlower.h"
#include "CanergyRuntimeComponents.h"
#include "GameplayDefinitionAssets.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

UCanergyWeaponSpecialComponent::UCanergyWeaponSpecialComponent()
{
	SetIsReplicatedByDefault(true);
}

float UCanergyWeaponSpecialComponent::GetCooldownRemaining() const
{
	const auto* Weapon = GetOwner()->FindComponentByClass<UCanergyToyWeaponControllerComponent>();
	const auto* Definition = Weapon ? Weapon->GetWeaponDefinition() : nullptr;
	const float* Ready = Definition ? ReadyTimes.Find(Definition->WeaponId) : nullptr;
	return Ready ? FMath::Max(0.0f, *Ready - GetWorld()->GetTimeSeconds()) : 0.0f;
}

bool UCanergyWeaponSpecialComponent::ActivateSpecial()
{
	if (!GetOwner()->HasAuthority()) { ServerActivateSpecial(); return false; }
	auto* Character = Cast<ACharacter>(GetOwner());
	const auto* Weapon = GetOwner()->FindComponentByClass<UCanergyToyWeaponControllerComponent>();
	const auto* Definition = Weapon ? Weapon->GetWeaponDefinition() : nullptr;
	const auto* Bubble = GetOwner()->FindComponentByClass<UCanergyBubbleRespawnComponent>();
	if (!Character || !Definition || (Bubble && Bubble->IsBubbled())) return false;
	if (!Definition->bHasBloomSpecial)
	{
		Feedback(FText::FromString(TEXT("这把玩具的特殊玩法尚在接入；棱花喷绘器可按 E 盛放")));
		return false;
	}
	if (GetCooldownRemaining() > 0.0f)
	{
		Feedback(FText::FromString(FString::Printf(TEXT("盛放冷却 · %.1f 秒"), GetCooldownRemaining())));
		return false;
	}
	FHitResult Ground;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CanergyBloomPlacement), false, Character);
	const FVector Feet = Character->GetActorLocation() - FVector::UpVector * Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	if (!GetWorld()->LineTraceSingleByChannel(Ground, Feet + FVector::UpVector * 20.0f,
		Feet - FVector::UpVector * 100.0f, ECC_Visibility, Params) || Ground.ImpactNormal.Z < 0.7f)
	{
		Feedback(FText::FromString(TEXT("盛放需要脚下有平整地面 · 未消耗冷却")));
		return false;
	}
	FActorSpawnParameters Spawn;
	Spawn.Owner = Character;
	Spawn.Instigator = Character;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UClass* FlowerClass = Definition->BloomActorClass.IsNull() ? ACanergyBounceFlower::StaticClass() : Definition->BloomActorClass.LoadSynchronous();
	if (!FlowerClass || !FlowerClass->IsChildOf(ACanergyBounceFlower::StaticClass())) return false;
	auto* Flower = GetWorld()->SpawnActor<ACanergyBounceFlower>(FlowerClass, Ground.ImpactPoint + FVector::UpVector * 8.0f,
		FRotator::ZeroRotator, Spawn);
	if (!Flower) return false;
	Flower->Configure(Definition->SpecialRadius, Definition->SpecialLaunchSpeed, Definition->SpecialLifetimeSeconds);
	if (LastFlower.IsValid()) LastFlower->Destroy(); // One active flower per owner, no world clutter.
	LastFlower = Flower;
	ReadyTimes.Add(Definition->WeaponId, GetWorld()->GetTimeSeconds() + FMath::Clamp(Definition->SpecialCooldownSeconds, 1.0f, 30.0f));
	Feedback(FText::FromString(TEXT("棱花盛放！临时弹射花已生成 · 玩家和轻型道具都能借力")));
	return true;
}

void UCanergyWeaponSpecialComponent::ServerActivateSpecial_Implementation() { ActivateSpecial(); }
void UCanergyWeaponSpecialComponent::Feedback(const FText& Message)
{
	if (const auto* Pawn = Cast<APawn>(GetOwner()); Pawn && Pawn->IsLocallyControlled()) ClientFeedback_Implementation(Message);
	else ClientFeedback(Message);
}
void UCanergyWeaponSpecialComponent::ClientFeedback_Implementation(const FText& Message)
{
	if (auto* Weapon = GetOwner()->FindComponentByClass<UCanergyToyWeaponControllerComponent>()) Weapon->OnWeaponFeedback.Broadcast(Message);
}
