// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HOGEnemyHUDBase.h"
#include "Blueprint/UserWidget.h"
#include "HOGEnemyWidget.generated.h"

class UHpWidget;
class UCharacterInfoWidget;
/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UHOGEnemyWidget : public UHOGEnemyHUDBase
{
public:
	GENERATED_BODY()
	
	virtual void SetEnemyHp(float CurHp, float MaxHp) override;
	virtual void SetEnemyName(const FName& Name) override;
	
protected:
	UPROPERTY(meta = (BindWidget))
	UCharacterInfoWidget* CharacterInfoWidget;
	
	UPROPERTY(meta = (BindWidget))
	UHpWidget* HpWidget;
	
	
};
