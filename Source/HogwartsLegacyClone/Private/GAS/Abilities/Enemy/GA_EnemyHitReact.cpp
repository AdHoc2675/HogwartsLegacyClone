// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/Enemy/GA_EnemyHitReact.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "Core/HOG_GameplayTags.h"
#include "Data/Enemy/DA_EnemyConfigBase.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_EnemyHitReact::UGA_EnemyHitReact()
{
	AbilityTags.AddTag(HOGGameplayTags::State_Hit);
	ActivationOwnedTags.AddTag(HOGGameplayTags::State_Hit);
	ActivationBlockedTags.AddTag(HOGGameplayTags::State_Hit);
	ActivationBlockedTags.AddTag(HOGGameplayTags::State_Attacking);
	CancelAbilitiesWithTag.AddTag(HOGGameplayTags::State_Attacking);
}

void UGA_EnemyHitReact::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(GetCharacter());
	if (!Enemy)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}
	
	UDA_EnemyConfigBase* Config = Enemy->GetEnemyConfig();
	
	if (Config && Config->HitReactMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this, NAME_None, Config->HitReactMontage);

		MontageTask->OnCompleted.AddDynamic(this, &UGA_EnemyHitReact::OnMontageCompleted);
		MontageTask->OnCancelled.AddDynamic(this, &UGA_EnemyHitReact::OnMontageCancelled);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_EnemyHitReact::OnMontageCancelled);
		MontageTask->ReadyForActivation();
	}
	else
	{
		// 몽타주 없으면 타이머로 경직
		Enemy->GetWorldTimerManager().SetTimer(
			StunTimerHandle,
			[this]()
			{
				EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			},
			StunDuration,
			false
		);
	}
	
}

void UGA_EnemyHitReact::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ACharacter* Character = GetCharacter())
	{
		Character->GetWorldTimerManager().ClearTimer(StunTimerHandle);
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_EnemyHitReact::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_EnemyHitReact::OnMontageCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
