// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Player/HOGPlayerHUDBase.h"
#include "HOGPlayerWidget.generated.h"

/**
 * 
 */
class USpellWidget;
class UHpWidget;

UCLASS()
class HOGWARTSLEGACYCLONE_API UHOGPlayerWidget : public UHOGPlayerHUDBase
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void SetCooldownActive(FGameplayTag SpellID, float CooldownSeconds) override;
	virtual void SetPlayerHP(float CurHP, float MaxHP) override;
	virtual void UnlockSpellSlot(FGameplayTag SpellID) override;

protected:
	UPROPERTY(meta = (BindWidget))
	UHpWidget* PlayerHPWidget;
	
	UPROPERTY(meta = (BindWidget))
	USpellWidget* SpellSlot_Accio;
	
	UPROPERTY(meta = (BindWidget))
	USpellWidget* SpellSlot_Incendio;
	
	UPROPERTY(meta = (BindWidget))
	USpellWidget* SpellSlot_Leviosa;
	
	UPROPERTY(meta = (BindWidget))
	USpellWidget* SpellSlot_Stupefy;
	
	UPROPERTY(meta = (BindWidget))
	USpellWidget* SpellSlot_Protego;
	
private:
	UPROPERTY()
	TMap<FGameplayTag, USpellWidget*> SpellSlots;
};
