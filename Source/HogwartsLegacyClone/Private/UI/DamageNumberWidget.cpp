// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/DamageNumberWidget.h"

#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Pool/DamageNumberPool.h"

void UDamageNumberWidget::FloatDamageNumber(float Damage, FVector2D ScreenPosition, UDamageNumberPool* InPool)
{
	StartLocation = ScreenPosition;
	DamageNumberPool = InPool;
	
	DamageText->SetText(FText::AsNumber(FMath::RoundToInt(Damage)));
	SetRenderOpacity(1.f);
	SetVisibility(ESlateVisibility::HitTestInvisible);

	StartTime = GetWorld()->GetTimeSeconds();

	GetWorld()->GetTimerManager().SetTimer(AnimTimerHandle, this, &UDamageNumberWidget::UpdateAnimation, 0.016f, true);
}

void UDamageNumberWidget::UpdateAnimation()
{
	float Elapsed = GetWorld()->GetTimeSeconds() - StartTime;
	float Alpha = FMath::Clamp(Elapsed / Duration, 0.0f, 1.0f);

	FVector2D NewLocation = StartLocation;
	// RiseSpeed에 비례하여 위로 상승
	// 처음은 Alpha가 0이므로 상승하지 않음
	NewLocation.Y -= RiseSpeed * Alpha;

	SetPositionInViewport(NewLocation);
	// 원 마이너스 하여 점점 Alpha가 작아지도록
	SetRenderOpacity(1.f - Alpha);
	
	if (Alpha >= 1.f)
	{
		GetWorld()->GetTimerManager().ClearTimer(AnimTimerHandle);
		
		SetVisibility(ESlateVisibility::Collapsed);
		// 현재 widgetComponent 반환
		if (DamageNumberPool)
		{
			DamageNumberPool->Release(this);
		}
	}
}
