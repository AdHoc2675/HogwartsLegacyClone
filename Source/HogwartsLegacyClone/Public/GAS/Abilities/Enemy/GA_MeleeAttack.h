// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/GA_EnemyBase.h"
#include "Data/Enemy/FEnemyAttackData.h"  

#include "GA_MeleeAttack.generated.h"

/**
 * 
 */

class UAnimMontage;

UCLASS()
class HOGWARTSLEGACYCLONE_API UGA_MeleeAttack : public UGA_EnemyBase
{
	GENERATED_BODY()
public:
	UGA_MeleeAttack();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	
	virtual int32 GetAttackTypeInt() const { return -1; }
	virtual void GetAttackRange(float& OutMinRange, float& OutMaxRange) const
	{
		OutMinRange = 0.f;
		OutMaxRange = 0.f;
	}

protected:
	virtual float GetMeleeAttackDamage() const;
	virtual UAnimMontage* GetAttackMontage() const;

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	UFUNCTION()
	void OnWeaponHit(FGameplayEventData Payload);
	
private:
	const FEnemyAttackData* GetAttackData() const;
};

