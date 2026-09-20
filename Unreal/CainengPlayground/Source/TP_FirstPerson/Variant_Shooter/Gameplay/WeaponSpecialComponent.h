#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponSpecialComponent.generated.h"

class ACanergyBounceFlower;

UCLASS(ClassGroup=(Canergy), meta=(BlueprintSpawnableComponent))
class TP_FIRSTPERSON_API UCanergyWeaponSpecialComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCanergyWeaponSpecialComponent();
	UFUNCTION(BlueprintCallable, Category="Canergy|Weapon")
	bool ActivateSpecial();
	float GetCooldownRemaining() const;
	ACanergyBounceFlower* GetLastFlower() const { return LastFlower.Get(); }
private:
	TMap<FName, float> ReadyTimes;
	TWeakObjectPtr<ACanergyBounceFlower> LastFlower;
	UFUNCTION(Server, Reliable)
	void ServerActivateSpecial();
	UFUNCTION(Client, Reliable)
	void ClientFeedback(const FText& Message);
	void Feedback(const FText& Message);
};
