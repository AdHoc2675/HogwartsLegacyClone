// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HOGEnemyHUDBase.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UHOGEnemyHUDBase : public UUserWidget
{
	GENERATED_BODY()
	
	public:
	virtual void SetEnemyHp(float CurHp, float MaxHp) {}
	virtual void SetEnemyName(const FName &Name) {}
};
