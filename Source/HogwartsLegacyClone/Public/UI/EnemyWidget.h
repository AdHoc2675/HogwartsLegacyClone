// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnemyWidget.generated.h"

class UHpWidget;
class UCharacterInfoWidget;
/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UEnemyWidget : public UUserWidget
{
public:
	GENERATED_BODY()
	
	void UpdateWidget(FText& Name, float HpRatio);
	void UpdateWidget(FText& Name, float NewHp, float MaxHp);
	
protected:
	UPROPERTY(meta = (BindWidget))
	UCharacterInfoWidget* CharacterInfoWidget;
	
	UPROPERTY(meta = (BindWidget))
	UHpWidget* HpWidget;
	
	
};
