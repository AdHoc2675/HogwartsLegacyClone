// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/Spell/Protego/GE_Protego.h"
#include "Core/HOG_GameplayTags.h"
#include "GameplayTagContainer.h"

UGE_Protego::UGE_Protego()
{
	//지속형으로 GE처리
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	
	//기본 지속시간
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.f));
	
	// Protego 활성 상태 태그 부여
	InheritableOwnedTagsContainer.AddTag(HOGGameplayTags::State_Spell_Protego_Active);
}
