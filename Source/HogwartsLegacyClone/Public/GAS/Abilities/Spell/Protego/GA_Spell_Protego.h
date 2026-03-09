// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/GA_SpellBase.h"
#include "GameplayEffectTypes.h"
#include "GA_Spell_Protego.generated.h"


class UGE_Protego;
class AProtegoActor;

/**
 * Protego 스펠 Ability
 * - 자신에게 GE_Protego 적용
 * - ProtegoActor 생성 및 Avatar에 부착
 * - 종료 시 보호막 Actor / GE 정리
 */

UCLASS()
class HOGWARTSLEGACYCLONE_API UGA_Spell_Protego : public UGA_SpellBase
{
	GENERATED_BODY()
	
public:
	UGA_Spell_Protego();
	
protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	/** Protego 활성 상태를 부여하는 GE 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Protego")
	TSubclassOf<UGE_Protego> ProtegoEffectClass;

	/** 시각/충돌 담당 Protego Actor 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Protego")
	TSubclassOf<AProtegoActor> ProtegoActorClass;

	/** 스폰된 Protego Actor 참조 */
	UPROPERTY()
	TObjectPtr<AProtegoActor> SpawnedProtegoActor;

	/** 적용한 GE 핸들 */
	FActiveGameplayEffectHandle ActiveProtegoEffectHandle;
	
};
