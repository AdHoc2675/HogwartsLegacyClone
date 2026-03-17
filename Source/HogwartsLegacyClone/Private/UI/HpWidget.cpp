// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HpWidget.h"

#include "Components/ProgressBar.h"

void UHpWidget::NativeDestruct()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	}
	
	Super::NativeDestruct();
}

void UHpWidget::SetHP(float NewHp, float MaxHp)
{
	if (MaxHp <= 0.0f) return;
	
	float Percentage = FMath::Clamp(NewHp / MaxHp, 0.0f, 1.0f);
	
	SetHP(Percentage);
}

void UHpWidget::SetHP(float HpRatio)
{
	TargetPercentage = HpRatio;
	
	if (!GetWorld()->GetTimerManager().IsTimerActive(TimerHandle))
	{
		GetWorld()->GetTimerManager().SetTimer(
			TimerHandle,
			this,
			&UHpWidget::UpdateBar,
			TimerRate,
			true);
	}
}

void UHpWidget::UpdateBar()
{
	if (FMath::IsNearlyEqual(CurrentPercentage, TargetPercentage, 0.001f))
	{
		CurrentPercentage = TargetPercentage;
		HPBar->SetPercent(CurrentPercentage);
		GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
		return;
	}
	
	CurrentPercentage = FMath::FInterpTo(CurrentPercentage, TargetPercentage, TimerRate, InterpSpeed);
	HPBar->SetPercent(CurrentPercentage);
}
