// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SpellWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"

void USpellWidget::UseSpell(const float Time)
{
	if (bUseSpell) return;

	bUseSpell = true;
	StartTime = GetWorld()->GetTimeSeconds();
	CooldownTime = Time;
	
	SpellIcon->SetColorAndOpacity(FLinearColor(0.1f, 0.1f, 0.1f, 1.f));
	GetWorld()->GetTimerManager().SetTimer(SpellTimerHandle, this, &USpellWidget::UpdateSpellCoolDown, 0.f, true);
}

void USpellWidget::UpdateSpellCoolDown()
{
	float ElapsedTime = GetWorld()->GetTimeSeconds() - StartTime;
	float Alpha = FMath::Clamp(ElapsedTime / CooldownTime, 0.f, 1.f);
	SpellProgressBar->SetPercent(Alpha);
	
	if (Alpha >= 1.f)
	{
		bUseSpell = false;
		SpellIcon->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 1.f));
		GetWorld()->GetTimerManager().ClearTimer(SpellTimerHandle);
	}
}
