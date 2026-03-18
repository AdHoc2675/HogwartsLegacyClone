// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "HOGPlayerHUDBase.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UHOGPlayerHUDBase : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void SetCooldownActive(FGameplayTag SpellID, float CooldownSeconds) {}
	virtual void SetPlayerHP(float CurrentHp, float MaxHp) {}
	virtual void UnlockSpellSlot(FGameplayTag SpellID) {}
};
