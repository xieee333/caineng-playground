#include "PhaseAbilityComponent.h"
#include "CanergyRuntimeComponents.h"
#include "CarryPrototype.h"
#include "ShooterCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Net/UnrealNetwork.h"

UCanergyPhaseAbilityComponent::UCanergyPhaseAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UCanergyPhaseAbilityComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UCanergyPhaseAbilityComponent, bPhased);
}

bool UCanergyPhaseAbilityComponent::ActivatePhase()
{
	auto* Character = Cast<AShooterCharacter>(GetOwner());
	if (!Character) return false;
	const FVector Forward = Character->GetControlRotation().Vector().GetSafeNormal2D();
	if (!GetOwner()->HasAuthority()) { ServerActivatePhase(Forward); return false; }
	const auto* Bubble = GetOwner()->FindComponentByClass<UCanergyBubbleRespawnComponent>();
	const auto* Carry = Cast<ACanergyCarryPrototype>(UGameplayStatics::GetActorOfClass(GetWorld(), ACanergyCarryPrototype::StaticClass()));
	const bool bCarryLocked = Carry && (Carry->IsFinished() || Carry->IsCarriedBy(Character));
	if ((Bubble && Bubble->IsBubbled()) || bCarryLocked)
	{
		Feedback(FText::FromString(TEXT("携带核心、泡泡回场或比赛结束时不能潜相")));
		UE_LOG(LogTemp, Display, TEXT("CANERGY_PHASE rejected gameplay-lock carry=%d bubble=%d finished=%d"),
			Carry && Carry->IsCarriedBy(Character), Bubble && Bubble->IsBubbled(), Carry && Carry->IsFinished());
		return false;
	}
	const FVector Start = Character->GetActorLocation();
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CanergyPhaseWall), false, Character);
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Forward * 260.0f, ECC_Visibility, Params)
		|| !Hit.GetActor() || !Hit.GetActor()->ActorHasTag(TEXT("CanergyPhaseWall")))
	{
		UE_LOG(LogTemp, Warning, TEXT("CANERGY_PHASE rejected no-tagged-wall owner=%s start=%s forward=%s hit=%s"),
			*GetOwner()->GetName(), *Start.ToString(), *Forward.ToString(), Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("None"));
		Feedback(FText::FromString(TEXT("潜相只能穿过带裂光标记的薄墙")));
		return false;
	}
	FVector Origin, Extent;
	Hit.GetActor()->GetActorBounds(false, Origin, Extent);
	const float ProjectedHalf = FMath::Abs(Forward.X) * Extent.X + FMath::Abs(Forward.Y) * Extent.Y;
	const float Thickness = ProjectedHalf * 2.0f;
	const float Radius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Exit = Hit.ImpactPoint + Forward * (Thickness + Radius + 24.0f);
	const bool bExitClear = !GetWorld()->OverlapBlockingTestByChannel(Exit, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeCapsule(Radius, HalfHeight), Params);
	if (!CainengGameRules::BeginPhase(State, false, true, Thickness, 180.0f, bExitClear))
	{
		Feedback(FText::FromString(State.CooldownRemaining > 0.0f
			? FString::Printf(TEXT("潜相冷却 · %.1f 秒"), State.CooldownRemaining)
			: TEXT("墙体过厚或出口被堵住，未消耗技能")));
		return false;
	}
	Character->SetActorLocation(Exit, false, nullptr, ETeleportType::TeleportPhysics);
	bPhased = true;
	AttackLockRemaining = 0.4f;
	ApplyPresentation();
	GetOwner()->ForceNetUpdate();
	UE_LOG(LogTemp, Display, TEXT("CANERGY_PHASE PASS wall=%s thickness=%.1f exit=%s"), *Hit.GetActor()->GetName(), Thickness, *Exit.ToString());
	Feedback(FText::FromString(TEXT("潜相突袭 · 短暂隐匿；开火或受击会立刻显形")));
	return true;
}

void UCanergyPhaseAbilityComponent::BreakPhase()
{
	if (!GetOwner()->HasAuthority()) { if (bPhased) ServerBreakPhase(); return; }
	if (!CainengGameRules::BreakPhase(State)) return;
	bPhased = false;
	ApplyPresentation();
	GetOwner()->ForceNetUpdate();
	UE_LOG(LogTemp, Display, TEXT("CANERGY_PHASE concealment broken"));
}

void UCanergyPhaseAbilityComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);
	AttackLockRemaining = FMath::Max(0.0f, AttackLockRemaining - DeltaSeconds);
	if (GetOwner()->HasAuthority())
	{
		const bool bWasActive = State.ActiveRemaining > 0.0f;
		CainengGameRules::AdvancePhase(State, DeltaSeconds);
		if (bWasActive && State.ActiveRemaining <= 0.0f)
		{
			bPhased = false;
			ApplyPresentation();
			GetOwner()->ForceNetUpdate();
			UE_LOG(LogTemp, Display, TEXT("CANERGY_PHASE concealment expired cooldown=%.1f"), State.CooldownRemaining);
		}
	}
	if (!bProbeTriggered && Cast<APawn>(GetOwner()) && Cast<APawn>(GetOwner())->IsPlayerControlled()
		&& Cast<APawn>(GetOwner())->IsLocallyControlled()
		&& FParse::Param(FCommandLine::Get(), TEXT("CanergyPhaseProbe")))
	{
		ProbeElapsed += DeltaSeconds;
		if (ProbeElapsed >= 10.0f)
		{
			bProbeTriggered = true;
			if (auto* Character = Cast<ACharacter>(GetOwner()); Character && Character->GetController())
				Character->GetController()->SetControlRotation(FRotator::ZeroRotator);
			UE_LOG(LogTemp, Display, TEXT("CANERGY_PHASE_PROBE request authority=%d local=%d"), GetOwner()->HasAuthority(), Cast<APawn>(GetOwner())->IsLocallyControlled());
			ActivatePhase();
		}
	}
	if (GetOwner()->HasAuthority() && Cast<APawn>(GetOwner()) && Cast<APawn>(GetOwner())->IsPlayerControlled()
		&& Cast<APawn>(GetOwner())->IsLocallyControlled()
		&& FParse::Param(FCommandLine::Get(), TEXT("CanergyPhaseCarryProbe")))
	{
		ProbeElapsed += DeltaSeconds;
		auto* Character = Cast<AShooterCharacter>(GetOwner());
		auto* Carry = Cast<ACanergyCarryPrototype>(UGameplayStatics::GetActorOfClass(GetWorld(), ACanergyCarryPrototype::StaticClass()));
		if (CarryLockProbeStage == 0 && ProbeElapsed >= 8.0f && Character && Carry)
		{
			Carry->Interact(Character);
			CarryLockProbeStage = 1;
			UE_LOG(LogTemp, Display, TEXT("CANERGY_PHASE_CARRY_PROBE pickup=%d"), Carry->IsCarriedBy(Character));
		}
		if (CarryLockProbeStage == 1 && ProbeElapsed >= 10.0f && Character && Carry)
		{
			const bool bActivated = ActivatePhase();
			const bool bPass = Carry->IsCarriedBy(Character) && !bActivated && !IsPhased();
			UE_LOG(LogTemp, Display, TEXT("CANERGY_PHASE_CARRY_PROBE %s"), bPass ? TEXT("PASS") : TEXT("FAIL"));
			CarryLockProbeStage = 2;
		}
	}
}

void UCanergyPhaseAbilityComponent::ApplyPresentation()
{
	if (auto* Character = Cast<ACharacter>(GetOwner())) Character->GetMesh()->SetVisibility(!bPhased, false);
}

void UCanergyPhaseAbilityComponent::OnRep_Phased()
{
	if (bPhased) AttackLockRemaining = 0.4f;
	ApplyPresentation();
	UE_LOG(LogTemp, Display, TEXT("CANERGY_PHASE_CLIENT phased=%d"), bPhased);
}
void UCanergyPhaseAbilityComponent::ServerActivatePhase_Implementation(FVector_NetQuantizeNormal AimDirection)
{
	UE_LOG(LogTemp, Display, TEXT("CANERGY_PHASE_SERVER rpc received owner=%s aim=%s"), *GetOwner()->GetName(), *AimDirection.ToString());
	auto* Character = Cast<ACharacter>(GetOwner());
	if (!Character || AimDirection.IsNearlyZero()) return;
	Character->GetController()->SetControlRotation(AimDirection.Rotation());
	ActivatePhase();
}
void UCanergyPhaseAbilityComponent::ServerBreakPhase_Implementation() { BreakPhase(); }
void UCanergyPhaseAbilityComponent::Feedback(const FText& Message) const
{
	if (auto* Weapon = GetOwner()->FindComponentByClass<UCanergyToyWeaponControllerComponent>()) Weapon->OnWeaponFeedback.Broadcast(Message);
}
