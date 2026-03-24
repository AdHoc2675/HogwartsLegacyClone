// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/GA_EnemyBase.h"
#include "GA_EnemyHitReact.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UGA_EnemyHitReact : public UGA_EnemyBase
{
	GENERATED_BODY()

public:
	UGA_EnemyHitReact();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	
	virtual void EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "HitReact")
	float StunDuration = 2.f;

	UFUNCTION()
	void OnMontageCompleted();
	UFUNCTION()
	void OnMontageCancelled();

private:
	FTimerHandle StunTimerHandle;
};