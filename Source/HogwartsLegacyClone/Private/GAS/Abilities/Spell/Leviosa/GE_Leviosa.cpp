// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/Spell/Leviosa/GE_Leviosa.h"
#include "Core/HOG_GameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

UGE_Leviosa::UGE_Leviosa()
{
	// 어빌리티(GA_Spell_Leviosa)에서 명시적으로 삭제할 때까지 무한 지속
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	// 'State.Debuff.Levitated' 태그 부여 컴포넌트 추가
	UTargetTagsGameplayEffectComponent* TargetTagsComponent =
		CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTagsComponent"));

	if (TargetTagsComponent)
	{
		GEComponents.Add(TargetTagsComponent);

		FInheritedTagContainer TagChanges = TargetTagsComponent->GetConfiguredTargetTagChanges();
		TagChanges.Added.AddTag(HOGGameplayTags::State_Spell_Leviosa_Levitated);
		TagChanges.CombinedTags.AddTag(HOGGameplayTags::State_Spell_Leviosa_Levitated);

		TargetTagsComponent->SetAndApplyTargetTagChanges(TagChanges);
	}
}

