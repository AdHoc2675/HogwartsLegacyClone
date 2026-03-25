// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/GA_HitReact.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "Core/HOG_GameplayTags.h"
#include "HOGDebugHelper.h"

UGA_HitReact::UGA_HitReact()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 이 Ability 자체가 "State.Hit" 태그로 검색/발동되게 한다.
	AbilityTags.AddTag(HOGGameplayTags::State_Hit);

	// 피격 중 상태가 필요하면 나중에 여기 추가 가능
	// ActivationOwnedTags.AddTag(HOGGameplayTags::State_Hit);
}

void UGA_HitReact::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	if (!ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	ACharacter* AvatarCharacter = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!AvatarCharacter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (!HitReactMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAnimInstance* AnimInstance = AvatarCharacter->GetMesh()
		? AvatarCharacter->GetMesh()->GetAnimInstance()
		: nullptr;

	if (!AnimInstance)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 이미 같은 피격 몽타주가 재생 중이면 중복 재생하지 않음
	if (AnimInstance->Montage_IsPlaying(HitReactMontage))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
		return;
	}

	const float PlayedLength = AnimInstance->Montage_Play(HitReactMontage, PlayRate);

	if (PlayedLength <= 0.0f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}