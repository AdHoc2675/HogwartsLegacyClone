// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Enemy/HOGEnemyWidget.h"
#include "UI/CharacterInfoWidget.h"
#include "UI/HpWidget.h"


void UHOGEnemyWidget::SetEnemyHp(float CurHp, float MaxHp)
{
	HpWidget->SetHP(CurHp, MaxHp);
}

void UHOGEnemyWidget::SetEnemyName(const FName& Name)
{
	FText NameText = FText::FromName(Name);
	CharacterInfoWidget->SetInfo(NameText);
}
