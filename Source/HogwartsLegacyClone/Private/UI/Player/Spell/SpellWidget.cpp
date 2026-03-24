// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Player/Spell/SpellWidget.h"

#include "GameplayTagContainer.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/ProgressBar.h"

void USpellWidget::UseSpell(FGameplayTag SpellID, float InCooldownTime)
{
	if (bUseSpell) return;

	bUseSpell = true;
	StartTime = GetWorld()->GetTimeSeconds();
	CooldownTime = InCooldownTime;

	SpellIcon->SetColorAndOpacity(FLinearColor(0.1f, 0.1f, 0.1f, 1.f));
	GetWorld()->GetTimerManager().SetTimer(SpellTimerHandle, this, &USpellWidget::UpdateSpellCoolDown, 0.016f, true);
}

void USpellWidget::UnLockSpell()
{
	SpellSlotLockOverlay->SetVisibility(ESlateVisibility::Visible);

	// 애니메이션 중복 발생을 대비해 기존 타이머 제거
	GetWorld()->GetTimerManager().ClearTimer(UnlockAnimTimerHandle);

	// 애니메이션 시작 시간 기록 후 타이머 시작 (약 60FPS 간격)
	UnlockAnimStartTime = GetWorld()->GetTimeSeconds();
	GetWorld()->GetTimerManager().SetTimer(UnlockAnimTimerHandle, this, &USpellWidget::UpdateUnlockAnimation, 0.016f, true);

}

void USpellWidget::UpdateUnlockAnimation()
{
	float ElapsedTime = GetWorld()->GetTimeSeconds() - UnlockAnimStartTime;
	float Alpha = FMath::Clamp(ElapsedTime / UnlockDuration, 0.f, 1.f);

	// FMath::Sin에 Alpha * PI를 넣으면 0 -> 1 -> 0의 값
	// 최대 크기(MaxScale - 1.f)만큼 더 커졌다가 다시 원래 크기로 돌아옴
	float CurrentScale = 1.0f + (FMath::Sin(Alpha * PI) * (MaxScale - 1.0f));

	if (SpellIcon)
	{
		SpellIcon->SetRenderScale(FVector2D(CurrentScale, CurrentScale));
	}

	// 애니메이션이 끝났을 때
	if (Alpha >= 1.f)
	{
		if (SpellIcon)
		{
			// 스케일을 정확히 1.0으로 초기화
			SpellIcon->SetRenderScale(FVector2D(1.f, 1.f));
		}

		// 타이머 종료
		GetWorld()->GetTimerManager().ClearTimer(UnlockAnimTimerHandle);
	}
}

void USpellWidget::UpdateSpellCoolDown()
{
	float ElapsedTime = GetWorld()->GetTimeSeconds() - StartTime;
	float Alpha = FMath::Clamp(ElapsedTime / CooldownTime, 0, 1.f);
	SpellProgressBar->SetPercent(Alpha);
	
	if (Alpha >= 1.f)
	{
		bUseSpell = false;
		SpellIcon->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 1.f));
		GetWorld()->GetTimerManager().ClearTimer(SpellTimerHandle);
	}
}
