// Copyright Epic Games, Inc. All Rights Reserved.

#include "RelayObjectiveWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"

void URelayObjectiveWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree) return;

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RelayHUDCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ObjectivePanel"));
	Panel->SetBrushColor(FLinearColor(0.018f, 0.035f, 0.060f, 0.88f));
	Panel->SetPadding(FMargin(16.0f, 11.0f));
	RootCanvas->AddChild(Panel);

	ObjectiveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ObjectiveText"));
	ObjectiveText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.95f, 1.0f, 1.0f)));
	ObjectiveText->SetAutoWrapText(false);
	ObjectiveText->SetWrapTextAt(728.0f);
	Panel->SetContent(ObjectiveText);

	if ((ObjectivePanelSlot = Cast<UCanvasPanelSlot>(Panel->Slot)))
	{
		ObjectivePanelSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		ObjectivePanelSlot->SetAlignment(FVector2D::ZeroVector);
		ObjectivePanelSlot->SetPosition(FVector2D(28.0f, 108.0f));
		ObjectivePanelSlot->SetSize(FVector2D(650.0f, 140.0f));
		ObjectivePanelSlot->SetZOrder(5);
		ObjectivePanelSlot->SetAutoSize(true);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetObjectiveState(0, false, false);
}

void URelayObjectiveWidget::SetObjectiveState(int32 RelayProgress, bool bJumpedCalibrationHurdle, bool bCompleted)
{
	if (!ObjectiveText) return;

	FString Objective;
	if (bCompleted)
	{
		Objective = TEXT("试炼完成 · 彩能中继已校准");
	}
	else if (RelayProgress < 3)
	{
		Objective = FString::Printf(TEXT("主线 · 中继校准 %d/3\n顺序：青 → 橙 → 洋红 · R 切色 · 左键喷绘\nWASD 移动 · 空格跳跃 · Shift 冲刺 · F 脉冲"), RelayProgress);
	}
	else if (bJumpedCalibrationHurdle)
	{
		Objective = TEXT("校准梁已越过\n前往绿色出口完成试炼\nWASD 移动 · 空格跳跃 · Shift 冲刺 · F 脉冲");
	}
	else
	{
		Objective = TEXT("中继 3/3 · 闸门已开\n从橙色梁前起跳，腾空越过 → 绿色出口\nWASD 移动 · 空格跳跃 · Shift 冲刺 · F 脉冲");
	}

	CurrentObjectiveText = FText::FromString(Objective);
	if (!bFeedbackActive)
	{
		ObjectiveText->SetText(CurrentObjectiveText);
	}
	ObjectivePanelBaseHeight = bCompleted ? 72.0f : 140.0f;
	ObjectivePanelBaseWidth = 650.0f;
	if (ObjectivePanelSlot)
	{
		ObjectivePanelSlot->SetSize(FVector2D(ObjectivePanelBaseWidth, ObjectivePanelBaseHeight + (bFeedbackActive ? 36.0f : 0.0f)));
	}
}

void URelayObjectiveWidget::SetGameplayStatus(const FText& StatusText)
{
	if (!ObjectiveText) return;
	CurrentObjectiveText = StatusText;
	ObjectivePanelBaseWidth = 760.0f;
	TArray<FString> Lines;
	StatusText.ToString().ParseIntoArrayLines(Lines);
	ObjectivePanelBaseHeight = FMath::Max(110.0f, 30.0f + Lines.Num() * 36.0f);
	ObjectiveText->SetText(bFeedbackActive
		? FText::Format(FText::FromString(TEXT("{0}\n{1}")), CurrentObjectiveText, CurrentFeedbackText)
		: CurrentObjectiveText);
	if (ObjectivePanelSlot) ObjectivePanelSlot->SetSize(FVector2D(ObjectivePanelBaseWidth, ObjectivePanelBaseHeight + (bFeedbackActive ? 36.0f : 0.0f)));
}

void URelayObjectiveWidget::ShowFeedback(const FText& Feedback, float Duration)
{
	if (!ObjectiveText) return;

	bFeedbackActive = true;
	CurrentFeedbackText = Feedback;
	ObjectiveText->SetText(FText::Format(FText::FromString(TEXT("{0}\n{1}")), CurrentObjectiveText, Feedback));
	if (ObjectivePanelSlot)
	{
		ObjectivePanelSlot->SetSize(FVector2D(ObjectivePanelBaseWidth, ObjectivePanelBaseHeight + 36.0f));
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FeedbackTimer);
		World->GetTimerManager().SetTimer(FeedbackTimer, this, &URelayObjectiveWidget::ClearFeedback,
			FMath::Max(0.1f, Duration), false);
	}
}

void URelayObjectiveWidget::ClearFeedback()
{
	bFeedbackActive = false;
	if (ObjectiveText)
	{
		ObjectiveText->SetText(CurrentObjectiveText);
	}
	if (ObjectivePanelSlot)
	{
		ObjectivePanelSlot->SetSize(FVector2D(ObjectivePanelBaseWidth, ObjectivePanelBaseHeight));
	}
}
