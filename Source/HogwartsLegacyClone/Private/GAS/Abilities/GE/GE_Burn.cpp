// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/GE/GE_Burn.h"

#include "Core/HOG_GameplayTags.h"

UGE_Burn::UGE_Burn()
{
	// 기본적으로 시간 기반 지속 효과
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// 기본값은 C++에서 과하게 고정하지 않고,
	// 실제 값은 BP_GE_Burn 에서 조정하는 것을 권장
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f));
	Period = 1.0f;

	// 중복 적용 정책은 우선 가장 무난한 값으로 시작
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	StackLimitCount = 1;

	// 화상 상태 태그 부여
	// 아래 태그는 HOG_GameplayTags 쪽에 정의되어 있어야 함
	InheritableOwnedTagsContainer.AddTag(HOGGameplayTags::State_Burned);

	
}
