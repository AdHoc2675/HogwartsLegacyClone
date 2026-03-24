// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageNumberWidget.generated.h"

class UTextBlock;
class UDamageNumberPool;
class UWidgetComponent;
/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void FloatDamageNumber(float Damage, FVector2D ScreenPosition, UDamageNumberPool* InPool);

protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* DamageText;

private:
	void UpdateAnimation();
	
	UPROPERTY()
	UDamageNumberPool* DamageNumberPool;
	
	FTimerHandle AnimTimerHandle;
	FVector2D StartLocation;
	float StartTime;
	float Duration = 1.f;
	float RiseSpeed = 80.f;
	
};
