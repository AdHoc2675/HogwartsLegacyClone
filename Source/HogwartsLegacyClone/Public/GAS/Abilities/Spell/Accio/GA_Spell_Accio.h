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

	// 토글 오프를 위해 입력 감지 함수 오버라이드
	virtual void InputPressed(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
	) override;

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 마법 시전 핵심 로직 (대상을 성공적으로 당기기 시작하면 true 반환)
	UFUNCTION(BlueprintCallable, Category = "HOG|Spell|Accio")
	bool FireAccio();

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

	// 대상에 따른 끌어당기기 속도 세분화
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Move")
	float EnemyPullSpeed = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Move")
	float InteractablePullSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Move")
	float DefaultPullSpeed = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Accio|Move")
	float StopDistance = 150.f;

	// ====== 런타임 상태 ======
	UPROPERTY(Transient)
	TObjectPtr<AActor> OriginalTarget; // 마법을 맞춘 대상(이펙트 스폰용)

	UPROPERTY(Transient)
	TObjectPtr<AActor> TargetToMove; // 실제로 이동할 대상 (적재물, 발판 등)

	UPROPERTY(Transient)
	TObjectPtr<AActor> PullDestination; // 도착 지점 (보통 플레이어지만, 타겟 지점일 수 있음)

	FTimerHandle PullTimerHandle;

	// 현재 타겟이 상호작용 가능한 물체인지 여부 (토글식 작동 판단용)
	bool bIsPullingInteractable = false;

	// 현재 적용 중인 당기기 속도
	float CurrentPullSpeed = 0.f;
	
protected:
	virtual void OnPreCastFacingFinished(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	void ExecuteAccioCast(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	);
};