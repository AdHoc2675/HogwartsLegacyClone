// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/Spell/Protego/GA_Spell_Protego.h"

#include "GAS/Abilities/Spell/Protego/GE_Protego.h"
#include "GAS/Abilities/Spell/Protego/ProtegoActor.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "HOGDebugHelper.h"

UGA_Spell_Protego::UGA_Spell_Protego()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Spell_Protego::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo,
                                        const FGameplayAbilityActivationInfo ActivationInfo,
                                        const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!AvatarActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 코스트/쿨다운/기본 Commit
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1) Protego 활성 GE 적용
	if (ProtegoEffectClass)
	{
		const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
			ProtegoEffectClass,
			GetAbilityLevel(Handle, ActorInfo)
		);

		if (SpecHandle.IsValid())
		{
			ActiveProtegoEffectHandle = ApplyGameplayEffectSpecToOwner(
				Handle,
				ActorInfo,
				ActivationInfo,
				SpecHandle
			);
		}
		else
		{
			Debug::Print(TEXT("[GA_Spell_Protego] ProtegoEffect SpecHandle invalid"), FColor::Red);
		}
	}
	else
	{
		Debug::Print(TEXT("[GA_Spell_Protego] ProtegoEffectClass is null"), FColor::Red);
	}

	// 2) Protego Actor 스폰
	if (ProtegoActorClass)
	{
		UWorld* World = AvatarActor->GetWorld();
		if (World)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = AvatarActor;
			SpawnParams.Instigator = Cast<APawn>(AvatarActor);
			SpawnParams.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			SpawnedProtegoActor = World->SpawnActor<AProtegoActor>(
				ProtegoActorClass,
				AvatarActor->GetActorLocation(),
				AvatarActor->GetActorRotation(),
				SpawnParams
			);

			if (SpawnedProtegoActor)
			{
				SpawnedProtegoActor->AttachToActor(
					AvatarActor,
					FAttachmentTransformRules::SnapToTargetNotIncludingScale
				);

				Debug::Print(TEXT("[GA_Spell_Protego] ProtegoActor spawned"), FColor::Green);
			}
			else
			{
				Debug::Print(TEXT("[GA_Spell_Protego] Failed to spawn ProtegoActor"), FColor::Red);
			}
		}
	}
	else
	{
		Debug::Print(TEXT("[GA_Spell_Protego] ProtegoActorClass is null"), FColor::Yellow);
	}

	// 지금 단계는 일단 지속형 Ability로 두고,
	// GE duration 종료 타이밍과 맞춰 추후 AbilityTask로 정리하면 된다.
	// 현재는 수동 종료 전까지 유지.
}

void UGA_Spell_Protego::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                   const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
                                   bool bWasCancelled)
{
	// 1) 스폰한 ProtegoActor 정리
	if (SpawnedProtegoActor)
	{
		SpawnedProtegoActor->Destroy();
		SpawnedProtegoActor = nullptr;
	}

	// 2) 적용한 GE 제거
	if (ActiveProtegoEffectHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveActiveGameplayEffect(ActiveProtegoEffectHandle);
		}

		ActiveProtegoEffectHandle.Invalidate();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
