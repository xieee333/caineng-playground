#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BounceFlower.generated.h"

class UStaticMeshComponent;

/** Temporary non-solid launch field; visuals may be replaced in a Blueprint child. */
UCLASS(Blueprintable)
class TP_FIRSTPERSON_API ACanergyBounceFlower : public AActor
{
	GENERATED_BODY()
public:
	ACanergyBounceFlower();
	virtual void Tick(float DeltaSeconds) override;
	void Configure(float Radius, float LaunchSpeed, float Lifetime);
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Canergy|Presentation")
	TObjectPtr<UStaticMeshComponent> Placeholder;
private:
	float EffectRadius = 220.0f;
	float UpSpeed = 950.0f;
	TMap<TWeakObjectPtr<AActor>, float> NextLaunchTime;
};
