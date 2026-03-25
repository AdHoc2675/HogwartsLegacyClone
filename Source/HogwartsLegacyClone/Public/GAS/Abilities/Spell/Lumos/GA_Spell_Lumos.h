// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/GA_SpellBase.h"
#include "GA_Spell_Lumos.generated.h"

class UAnimMontage;
class USoundBase;
class UPointLightComponent;
class UNiagaraSystem;
class UNiagaraComponent;

/**
 * Lumos 스펠 Ability
 * - 사용시 광원 생성	
 */

UCLASS()
class HOGWARTSLEGACYCLONE_API UGA_Spell_Lumos : public UGA_SpellBase
{
	GENERATED_BODY()
	
public:
	UGA_Spell_Lumos();

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

protected:
	// Start 애니메이션 완료 시 호출
	UFUNCTION()
	void OnStartMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// Stop 애니메이션 완료 시 호출 (불 끄고 어빌리티 완전 종료)
	UFUNCTION()
	void OnStopMontageEnded(UAnimMontage* Montage, bool bInterrupted);

protected:
	// ====== Lumos 설정 ======
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Lumos|Anim")
	TObjectPtr<UAnimMontage> CastMontage;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Lumos|Anim")
	TObjectPtr<UAnimMontage> StopMontage;

	// 기존 보이스 사운드
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Lumos|Sound")
	TObjectPtr<USoundBase> CastVoiceSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HOG|Spell|Lumos|Sound")
	FString CastSubtitleText = TEXT("루모스!");

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Lumos|Sound")
	TObjectPtr<USoundBase> EndVoiceSound;

	// 마법 효과음 (SFX)
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Lumos|Sound")
	TObjectPtr<USoundBase> CastSound;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Lumos|Sound")
	TObjectPtr<USoundBase> EndSound;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Lumos|Visual")
	TObjectPtr<UNiagaraSystem> LumosVFX;

	// ====== 광원 조율 ======
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Lumos|Light")
	FLinearColor LightColor = FColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Lumos|Light")
	float LightIntensity = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Lumos|Light")
	float LightAttenuationRadius = 1500.f;

	// ====== 런타임 상태 ======
	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> SpawnedLight; // 머리 위 메인 조명

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> SpawnedVFX;

	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> WandLight; // 지팡이 끝 보조 조명

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> WandVFX;

	UPROPERTY(Transient)
	bool bIsActiveLumos = false;
};
