// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/GE/GE_Stun.h"

#include "Core/HOG_GameplayTags.h"

UGE_Stun::UGE_Stun()
{
	// 기절은 일정 시간 유지되는 상태이므로 Duration 기반
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// 기본 지속시간
	// 나중에 BP 자식에서 쉽게 바꿀 수 있게 기본값만 넣는다.
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.5f));

	// 적용 중 대상이 보유할 상태 태그
	InheritableOwnedTagsContainer.AddTag(HOGGameplayTags::State_Stunned);

	// 필요 시 나중에 아래도 추가 가능:
	// - GrantedAbilities
	// - OngoingTagRequirements
	// - RemoveGameplayEffectsWithTags
}