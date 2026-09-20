#include "WallTraversalComponent.h"
#include "CanergyRuntimeComponents.h"
#include "TraversalRules.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

UCanergyWallTraversalComponent::UCanergyWallTraversalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UCanergyWallTraversalComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<ACharacter>(GetOwner());
	if (Character.IsValid()) Character->GetCharacterMovement()->AddTickPrerequisiteComponent(this);
}

void UCanergyWallTraversalComponent::SetDiveHeld(bool bNewHeld)
{
	bHeld = bNewHeld;
	if (!bHeld) Cancel();
}

bool UCanergyWallTraversalComponent::HandleMove(float Right, float Forward)
{
	ForwardIntent = FMath::Clamp(Forward, -1.0f, 1.0f);
	if (!bActive || !Character.IsValid()) return false;
	const FVector WallRight = FVector::CrossProduct(WallNormal, FVector::UpVector).GetSafeNormal();
	const FVector Input = (WallRight * FMath::Clamp(Right, -1.0f, 1.0f)
		+ FVector::UpVector * ForwardIntent).GetClampedToMaxSize(1.0f);
	Character->AddMovementInput(Input.GetSafeNormal(), Input.Size());
	return true;
}

bool UCanergyWallTraversalComponent::FindLiquidWall(FHitResult& Hit) const
{
	if (!Character.IsValid()) return false;
	const FVector Direction = bActive ? -WallNormal : Character->GetActorForwardVector().GetSafeNormal2D();
	const float Reach = Character->GetCapsuleComponent()->GetScaledCapsuleRadius() + ContactReach;
	const FVector Start = Character->GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CanergyWallContact), false, Character.Get());
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Direction * Reach, ECC_Visibility, Params)) return false;
	const auto* Surface = Hit.GetActor() ? Hit.GetActor()->FindComponentByClass<UCanergyPaintableSurfaceComponent>() : nullptr;
	return CainengGameRules::IsClimbableLiquidWall(Surface && Surface->GetSurfaceKind() == ECanergySurfaceKind::Liquid,
		Hit.ImpactNormal.Z);
}

void UCanergyWallTraversalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const float RequestedForward = ForwardIntent;
	ForwardIntent = 0.0f; // InputAction Triggered does not emit a final zero when released.
	ReattachDelay = FMath::Max(0.0f, ReattachDelay - DeltaTime);
	if (!Character.IsValid()) return;
	auto* Movement = Character->GetCharacterMovement();
	const auto* Bubble = Character->FindComponentByClass<UCanergyBubbleRespawnComponent>();
	if (!bHeld || (Bubble && Bubble->IsBubbled()) || Movement->MovementMode == MOVE_None)
	{
		Cancel(); return;
	}
	if (bActive && Movement->MovementMode != MOVE_Flying) { Cancel(); return; }
	if (!bActive && (ReattachDelay > 0.0f || RequestedForward <= 0.1f
		|| (!Movement->IsMovingOnGround() && !Movement->IsFalling()))) return;
	FHitResult Hit;
	if (!FindLiquidWall(Hit)) { Cancel(); return; }
	WallNormal = Hit.ImpactNormal.GetSafeNormal2D();
	if (!bActive)
	{
		bActive = true;
		PreviousFlySpeed = Movement->MaxFlySpeed;
		Character->UnCrouch();
		Movement->StopMovementImmediately();
		Movement->SetMovementMode(MOVE_Flying);
	}
	Movement->MaxFlySpeed = WallSpeed;
	// CharacterMovement performs collision sweeps/sliding; never teleport through walls.
}

void UCanergyWallTraversalComponent::Cancel()
{
	if (!bActive) return;
	bActive = false;
	ReattachDelay = 0.25f;
	if (!Character.IsValid()) return;
	auto* Movement = Character->GetCharacterMovement();
	Movement->MaxFlySpeed = PreviousFlySpeed;
	// Do not override a bubble/ability that has explicitly disabled movement.
	if (Movement->MovementMode == MOVE_Flying) Movement->SetMovementMode(MOVE_Falling);
}

bool UCanergyWallTraversalComponent::TryWallJump()
{
	if (!bActive || !Character.IsValid()) return false;
	const FVector JumpVelocity = WallNormal * 500.0f + FVector::UpVector * 500.0f;
	Cancel();
	ReattachDelay = 0.4f;
	Character->LaunchCharacter(JumpVelocity, true, true);
	return true;
}
