// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Player/HOGPlayerWidget.h"
#include "UI/HpWidget.h"
#include "Core/HOG_GameplayTags.h"
#include "UI/Player/Spell/SpellWidget.h"

void UHOGPlayerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	SpellSlots.Add(HOGGameplayTags::Spell_Accio, SpellSlot_Accio);
	SpellSlots.Add(HOGGameplayTags::Spell_Incendio, SpellSlot_Incendio);
	SpellSlots.Add(HOGGameplayTags::Spell_Leviosa, SpellSlot_Leviosa);
	SpellSlots.Add(HOGGameplayTags::Spell_Stupefy, SpellSlot_Stupefy);
	SpellSlots.Add(HOGGameplayTags::Spell_Protego, SpellSlot_Protego);
}

void UHOGPlayerWidget::UnlockSpellSlot(FGameplayTag SpellID)
{
	if (USpellWidget* Found = SpellSlots.FindRef(SpellID))
	{
		Found->UnLockSpell();	
	}
}

void UHOGPlayerWidget::SetCooldownActive(FGameplayTag SpellID, float CooldownSeconds)
{
	if (USpellWidget* Found = SpellSlots.FindRef(SpellID))
	{
		Found->UseSpell(SpellID, CooldownSeconds);
	}
}

void UHOGPlayerWidget::SetPlayerHP(float CurHP, float MaxHP)
{
	if (PlayerHPWidget)
	{
		PlayerHPWidget->SetHP(CurHP, MaxHP);
	}
}
