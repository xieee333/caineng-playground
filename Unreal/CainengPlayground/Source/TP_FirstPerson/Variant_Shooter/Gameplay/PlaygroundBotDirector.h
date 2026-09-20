#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlaygroundBotDirector.generated.h"

class AShooterCharacter;

/** Local six-participant prototype. Bots use the same character commands as the player. */
UCLASS()
class TP_FIRSTPERSON_API ACanergyPlaygroundBotDirector : public AActor
{
	GENERATED_BODY()
public:
	ACanergyPlaygroundBotDirector();
	virtual void Tick(float DeltaSeconds) override;
private:
	struct FBotState
	{
		TWeakObjectPtr<AShooterCharacter> Pawn;
		FVector LastLocation = FVector::ZeroVector;
		float Heading = 0.0f;
		float NextThink = 0.0f;
		float NextShot = 0.0f;
		float StopShot = 0.0f;
		float NextJump = 0.0f;
		float Distance = 0.0f;
		int32 FireCommands = 0;
	};
	TArray<FBotState> Bots;
	FRandomStream Random{260920};
	float Elapsed = 0.0f;
	bool bSpawnAttempted = false;
	bool bProbe = false;
	bool bProbeDamageSent = false;
	bool bProbeReturned = false;
	bool bReported = false;
	void SpawnParticipants();
};
