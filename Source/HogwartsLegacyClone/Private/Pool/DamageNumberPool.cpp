// Fill out your copyright notice in the Description page of Project Settings.


#include "Pool/DamageNumberPool.h"
#include "Components/WidgetComponent.h"

void UDamageNumberPool::InitPool(APlayerController* InPlayerController, TSubclassOf<UUserWidget> InWidgetClass,
                                 int32 PoolSize)
{
	PlayerController = InPlayerController;
	WidgetClass = InWidgetClass;

	Pool.Reserve(PoolSize);
	ExpandPool(PoolSize);
}

// 풀에서 Component를 꺼내서 외부에서 정해진 위치로 설정후 반환
UWidgetComponent* UDamageNumberPool::Acquire(USceneComponent* AttachTarget, FVector Offset)
{
	UWidgetComponent* Component = nullptr;
	
	if (!TryGetComponent(Component))
	{
		return nullptr;
	}
	
	Component->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform);

	Component->SetRelativeLocation(Offset);
	
	if (!Component->IsRegistered())
	{
		Component->Rename(nullptr, AttachTarget->GetOwner());
		Component->RegisterComponent();
		Component->InitWidget();
	}

	Component->SetVisibility(true);
	Component->SetHiddenInGame(false);

	return Component;
}

// Enemy에 컴포넌트를 떼어내고 Visibility false
void UDamageNumberPool::Release(UWidgetComponent* Component)
{
	if (!Component) return;

	Component->SetVisibility(false);
	Component->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
	Pool.Push(Component);
}

// 컴포넌트 생성
UWidgetComponent* UDamageNumberPool::CreateWidgetComponent()
{
	UWidgetComponent* Component = NewObject<UWidgetComponent>(PlayerController);
	Component->SetWidgetClass(WidgetClass);
	Component->SetWidgetSpace(EWidgetSpace::Screen);
	Component->SetDrawAtDesiredSize(true);
	Component->SetVisibility(false);

	return Component;
}

// Expand Pool size
void UDamageNumberPool::ExpandPool(int32 Size)
{
	for (int32 i = 0; i < Size; i++)
	{
		Pool.Push(CreateWidgetComponent());
	}
}

bool UDamageNumberPool::TryGetComponent(UWidgetComponent*& OutComponent)
{
	if (Pool.IsEmpty())
	{
		ExpandPool(5);
	}

	OutComponent = Pool.Pop();

	return OutComponent != nullptr;
}
