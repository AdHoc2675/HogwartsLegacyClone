// Fill out your copyright notice in the Description page of Project Settings.

#include "Core/HOG_GameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GAS/Abilities/Enemy/GA_MeleeAttack.h"

UGA_MeleeAttack::UGA_MeleeAttack()
{
	// 어빌리티 태그 등록
	AbilityTags.AddTag(HOGGameplayTags::Ability_Attack);
	
	// 어빌리티 시작되면 자동 추가, 끝나면 자동 제거
	ActivationOwnedTags.AddTag(HOGGameplayTags::State_Attacking);
	
	// 공격 중 일 때, Block
	ActivationBlockedTags.AddTag(HOGGameplayTags::State_Attacking);
	
	// 피격 중 일 때, Block 
	ActivationBlockedTags.AddTag(HOGGameplayTags::State_Hit);
}

void UGA_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!AttackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		AttackMontage);
	
	// 몽타주가 끝날 때
	MontageTask->OnCompleted.AddDynamic(this, &UGA_MeleeAttack::OnMontageCompleted);
	
	// 몽타주 예외 상황
	// ex) 피격 etc
	MontageTask->OnCancelled.AddDynamic(this, &UGA_MeleeAttack::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_MeleeAttack::OnMontageCancelled);
	
	MontageTask->ReadyForActivation();
}

void UGA_MeleeAttack::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_MeleeAttack::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
