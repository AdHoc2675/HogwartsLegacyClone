// Fill out your copyright notice in the Description page of Project Settings.


#include "Pool/DamageNumberPool.h"

#include "UI/DamageNumberWidget.h"
#include "Components/WidgetComponent.h"

void UDamageNumberPool::InitPool(APlayerController* InPlayerController, TSubclassOf<UUserWidget> InWidgetClass,
                                 int32 PoolSize)
{
	PlayerController = InPlayerController;
	WidgetClass = InWidgetClass;

	Pool.Reserve(PoolSize);
	ExpandPool(PoolSize);
}

void UDamageNumberPool::ShowDamage(float Damage, FVector WorldLocation)
{
	if (!PlayerController) return;

	FVector2D ScreenPosition;
	if (PlayerController->ProjectWorldLocationToScreen(WorldLocation, ScreenPosition))
	{
		UDamageNumberWidget* Widget = nullptr;
		if (TryGetWidget(Widget))
		{
			Widget->FloatDamageNumber(Damage, ScreenPosition, this);
		}
	}
}

void UDamageNumberPool::Release(UDamageNumberWidget* Widget)
{
	if (Widget)
	{
		Pool.Push(Widget);
	}
}

// 위젯 생성
UDamageNumberWidget* UDamageNumberPool::CreateDamageNumberWidget()
{
	if (!PlayerController || !WidgetClass) return nullptr;

	UDamageNumberWidget* NewWidget = CreateWidget<UDamageNumberWidget>(PlayerController, WidgetClass);
	if (NewWidget)
	{
		NewWidget->AddToViewport(100);
		NewWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	return NewWidget;
}

// 풀 사이즈 키우기
void UDamageNumberPool::ExpandPool(int32 Size)
{
	for (int32 i = 0; i < Size; i++)
	{
		Pool.Push(CreateDamageNumberWidget());
	}
}

bool UDamageNumberPool::TryGetWidget(UDamageNumberWidget*& OutWidget)
{
	if (Pool.IsEmpty())
	{
		ExpandPool(5);
	}

	OutWidget = Pool.Pop();
	return OutWidget != nullptr;
}
