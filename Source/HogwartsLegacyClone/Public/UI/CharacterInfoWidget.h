// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CharacterInfoWidget.generated.h"

/**
 * 
 */
class UTextBlock;

UCLASS()
class HOGWARTSLEGACYCLONE_API UCharacterInfoWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetInfo(FText& Name);
	
protected:
	UPROPERTY(meta = (BindWidget))
	UTextBlock* CharacterName;
};
