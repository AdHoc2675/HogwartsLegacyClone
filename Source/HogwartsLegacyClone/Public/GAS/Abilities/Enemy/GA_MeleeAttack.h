// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/GA_EnemyBase.h"
#include "GA_MeleeAttack.generated.h"

/**
 * 
 */

class UAnimMontage;

UCLASS()
class HOGWARTSLEGACYCLONE_API UGA_MeleeAttack : public UGA_EnemyBase
{
	GENERATED_BODY()

	UGA_MeleeAttack();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* AttackMontage;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();
};
