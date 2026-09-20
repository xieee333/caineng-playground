// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "RelayObjectiveWidget.generated.h"

class UTextBlock;
class UCanvasPanelSlot;

/** Persistent, player-facing objective and control hint for the relay prototype. */
UCLASS()
class TP_FIRSTPERSON_API URelayObjectiveWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetObjectiveState(int32 RelayProgress, bool bJumpedCalibrationHurdle, bool bCompleted);
	void SetGameplayStatus(const FText& StatusText);
	void ShowFeedback(const FText& Feedback, float Duration = 1.8f);

protected:
	virtual void NativeOnInitialized() override;

private:
	TObjectPtr<UTextBlock> ObjectiveText;
	TObjectPtr<UCanvasPanelSlot> ObjectivePanelSlot;
	FText CurrentObjectiveText;
	FText CurrentFeedbackText;
	FTimerHandle FeedbackTimer;
	float ObjectivePanelBaseWidth = 650.0f;
	float ObjectivePanelBaseHeight = 140.0f;
	bool bFeedbackActive = false;

	void ClearFeedback();
};
