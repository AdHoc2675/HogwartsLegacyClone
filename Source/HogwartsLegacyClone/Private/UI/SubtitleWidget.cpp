// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SubtitleWidget.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

void USubtitleWidget::ShowSubtitle(const FString& Text, float Duration)
{
	if (!SubtitleText) return;

	// 텍스트 지정 및 화면에 보이게 변경
	SubtitleText->SetText(FText::FromString(Text));
	SetVisibility(ESlateVisibility::HitTestInvisible);

	// 기존 타이머 무효화 (연속으로 호출될 경우 대비)
	GetWorld()->GetTimerManager().ClearTimer(SubtitleTimerHandle);

	// Duration 이후에 텍스트를 숨기는 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(
		SubtitleTimerHandle, 
		this, 
		&USubtitleWidget::HideSubtitle, 
		Duration, 
		false
	);
}

void USubtitleWidget::HideSubtitle()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

