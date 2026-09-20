#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhaseRules.h"
#include "PhaseAbilityComponent.generated.h"

UCLASS(ClassGroup=(Canergy), meta=(BlueprintSpawnableComponent))
class TP_FIRSTPERSON_API UCanergyPhaseAbilityComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCanergyPhaseAbilityComponent();
	virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION(BlueprintCallable, Category="Canergy|Ability") bool ActivatePhase();
	UFUNCTION(BlueprintCallable, Category="Canergy|Ability") void BreakPhase();
	bool IsPhased() const { return bPhased; }
	bool CanAttack() const { return AttackLockRemaining <= 0.0f; }
	float GetCooldownRemaining() const { return State.CooldownRemaining; }
private:
	CainengGameRules::FPhaseState State;
	UPROPERTY(ReplicatedUsing=OnRep_Phased) bool bPhased = false;
	float ProbeElapsed = 0.0f;
	float AttackLockRemaining = 0.0f;
	bool bProbeTriggered = false;
	int32 CarryLockProbeStage = 0;
	UFUNCTION() void OnRep_Phased();
	UFUNCTION(Server, Reliable) void ServerActivatePhase(FVector_NetQuantizeNormal AimDirection);
	UFUNCTION(Server, Reliable) void ServerBreakPhase();
	void ApplyPresentation();
	void Feedback(const FText& Message) const;
};
