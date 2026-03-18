// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/EnemyWidget.h"
#include "UI/CharacterInfoWidget.h"
#include "UI/HpWidget.h"


void UEnemyWidget::UpdateWidget(FText& Name, float HpRatio)
{
	CharacterInfoWidget->SetInfo(Name);
	HpWidget->SetHP(HpRatio);
}

void UEnemyWidget::UpdateWidget(FText& Name, float NewHp, float MaxHp)
{
	CharacterInfoWidget->SetInfo(Name);
	HpWidget->SetHP(NewHp, MaxHp);
}
