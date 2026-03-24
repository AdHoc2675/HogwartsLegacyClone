// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DamageNumberPool.generated.h"

class UDamageNumberWidget;
/**
 * 
 */
class UWidgetComponent;


UCLASS()
class HOGWARTSLEGACYCLONE_API UDamageNumberPool : public UObject
{
	GENERATED_BODY()

public:
	void InitPool(APlayerController* InPlayerController, TSubclassOf<UUserWidget> InWidgetClass, int32 PoolSize = 20);
	
	void ShowDamage(float Damage, FVector WorldLocation);
	
	void Release(UDamageNumberWidget* Widget);

private:
	UDamageNumberWidget* CreateDamageNumberWidget();
	void ExpandPool(int32 Size);
	bool TryGetWidget(UDamageNumberWidget*& OutWidget);
	
	UPROPERTY()
	TArray<UDamageNumberWidget*> Pool;
	
	UPROPERTY()
	TSubclassOf<UUserWidget> WidgetClass;
	
	UPROPERTY()
	APlayerController* PlayerController;
};
