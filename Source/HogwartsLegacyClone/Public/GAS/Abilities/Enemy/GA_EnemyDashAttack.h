// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/Enemy/GA_MeleeAttack.h"
#include "GA_EnemyDashAttack.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UGA_EnemyDashAttack : public UGA_MeleeAttack
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashForce = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashHitRadius = 150.f;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

private:
	void CheckBodyHit();
	
	UFUNCTION()
	void OnDashFinished();

	FTimerHandle TimerHandle;
	bool bHasHit = false;
};
