// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/Spell/Protego/GE_Protego.h"
#include "Core/HOG_GameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "GameplayTagContainer.h"

UGE_Protego::UGE_Protego()
{
	// 지속형 GE 처리
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// 기본 지속시간
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.f));

	// Protego 활성 상태 태그 부여
	UTargetTagsGameplayEffectComponent* TargetTagsComponent =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));

	if (TargetTagsComponent)
	{
		GEComponents.Add(TargetTagsComponent);

		FInheritedTagContainer TagChanges = TargetTagsComponent->GetConfiguredTargetTagChanges();
		TagChanges.Added.AddTag(HOGGameplayTags::State_Spell_Protego_Active);
		TagChanges.CombinedTags.AddTag(HOGGameplayTags::State_Spell_Protego_Active);

		TargetTagsComponent->SetAndApplyTargetTagChanges(TagChanges);
	}
}
