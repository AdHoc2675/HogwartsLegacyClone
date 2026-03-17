// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/CharacterInfoWidget.h"

#include "Components/TextBlock.h"

void UCharacterInfoWidget::SetInfo(FText& Name)
{
	CharacterName->SetText(Name);
}
