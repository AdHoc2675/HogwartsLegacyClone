// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DamageNumberPool.generated.h"

/**
 * 
 */
class UWidgetComponent;


UCLASS()
class HOGWARTSLEGACYCLONE_API UDamageNumberPool : public UObject
{
	GENERATED_BODY()

public:
	void InitPool(APlayerController* InPlayerController, TSubclassOf<UUserWidget> InWidgetClass, int32 PoolSize = 10);
	UWidgetComponent* Acquire(USceneComponent* AttachTarget, FVector Offset);
	void Release(UWidgetComponent* Component);

private:
	UWidgetComponent* CreateWidgetComponent();
	void ExpandPool(int32 Size);
	bool TryGetComponent(UWidgetComponent*& OutComponent);
	
	UPROPERTY()
	TArray<UWidgetComponent*> Pool;
	
	UPROPERTY()
	TSubclassOf<UUserWidget> WidgetClass;
	
	UPROPERTY()
	APlayerController* PlayerController;
};
