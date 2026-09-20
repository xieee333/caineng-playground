#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WallTraversalComponent.generated.h"

class ACharacter;

/** Surface traversal adapter, independent of character meshes and weapon presentation. */
UCLASS(ClassGroup=(Canergy), meta=(BlueprintSpawnableComponent))
class TP_FIRSTPERSON_API UCanergyWallTraversalComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UCanergyWallTraversalComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void SetDiveHeld(bool bHeld);
	bool HandleMove(float Right, float Forward);
	bool TryWallJump();
	bool IsWallTraversing() const { return bActive; }
	void Cancel();

	UPROPERTY(EditAnywhere, Category="Canergy|Traversal", meta=(ClampMin="100", ClampMax="1500"))
	float WallSpeed = 720.0f;
	UPROPERTY(EditAnywhere, Category="Canergy|Traversal", meta=(ClampMin="5", ClampMax="80"))
	float ContactReach = 35.0f;
private:
	TWeakObjectPtr<ACharacter> Character;
	FVector WallNormal = FVector::ZeroVector;
	float PreviousFlySpeed = 600.0f;
	float ReattachDelay = 0.0f;
	float ForwardIntent = 0.0f;
	bool bHeld = false;
	bool bActive = false;
	bool FindLiquidWall(FHitResult& Hit) const;
};
