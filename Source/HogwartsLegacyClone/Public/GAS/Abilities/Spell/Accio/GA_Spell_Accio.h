// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/GA_SpellBase.h"
#include "GA_Spell_Accio.generated.h"

class UAnimMontage;
class USoundBase;
class UNiagaraSystem;

UCLASS()
class HOGWARTSLEGACYCLONE_API UGA_Spell_Accio : public UGA_SpellBase
{
	GENERATED_BODY()

public:
	UGA_Spell_Accio();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 마법 시전 핵심 로직
	UFUNCTION(BlueprintCallable, Category = "HOG|Spell|Accio")
	void FireAccio();

	// 타이머 루프: 지속적으로 끌어당기기 연산
	void UpdatePulling();

protected:
	// ====== Accio 설정 ======
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Anim")
	TObjectPtr<UAnimMontage> CastMontage;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Sound")
	TObjectPtr<USoundBase> CastVoiceSound;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Visual")
	TObjectPtr<UNiagaraSystem> AccioVFX;

	// 끌어당기는 속도
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Move")
	float PullSpeed = 1000.f;

	// 플레이어와의 최소 거리
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Move")
	float StopDistance = 150.f;

	// ====== 런타임 상태 ======
	UPROPERTY(Transient)
	TObjectPtr<AActor> PulledTarget;

	FTimerHandle PullTimerHandle;
};