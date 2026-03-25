// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/GA_SpellBase.h"
#include "GA_Spell_Leviosa.generated.h"

class UAnimMontage;
class UGameplayEffect;
class UNiagaraSystem;
class USoundBase;

/**
 *
 * Leviosa (부유 마법)
 * - 대상(Movable Object 또는 적)을 타겟팅하여 공중으로 띄움.
 * - 향후 이동, F/V/Q/E 키를 통한 세부 거리/회전 제어 로직이 추가될 예정
 * - 혹은 간단히 대상을 위로 띄우는 정도로만 구현
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UGA_Spell_Leviosa : public UGA_SpellBase
{
	GENERATED_BODY()

public:
	UGA_Spell_Leviosa();

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

	// 애니메이션 재생 종료 콜백
	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 타겟 부유 시작 상태 진입 (태그 바인딩 및 상태 적용용)
	UFUNCTION(BlueprintCallable, Category = "HOG|Spell|Leviosa")
	bool StartLevitation();

	// 지속 시간 종료시 어빌리티를 종료
	UFUNCTION()
	void OnLevitationDurationEnded();

protected:
	// ====== Leviosa 옵션 ======

	// 캐스팅 애니메이션
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Leviosa|Anim")
	TObjectPtr<UAnimMontage> CastMontage;

	// 대상을 띄우는 상태이상 (이동 불가 등 태그 부여)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HOG|Spell|Leviosa|Effect")
	TSubclassOf<UGameplayEffect> LevitationEffectClass;

	// 마법 지속 시간 (초 단위) - 상승 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HOG|Spell|Leviosa|Time")
	float LevitationDuration = 1.0f;

	// 허공에 머무는 유지 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HOG|Spell|Leviosa|Time")
	float LevitationHoverDuration = 2.5f; 

	// ====== 시각 및 사운드 효과 ======
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Leviosa|Visual")
	TObjectPtr<UNiagaraSystem> LevitateVFX;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Leviosa|Sound")
	TObjectPtr<USoundBase> CastVoiceSound;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Leviosa|Sound")
	TObjectPtr<USoundBase> LevitateSound;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Leviosa|Sound")
	FString CastSubtitleText = TEXT("윙가디움 레비오사!");

	// ====== 런타임 트래킹 ======

	// 현재 제어 중인 대상 (런타임 트래킹 용도)
	UPROPERTY(Transient)
	TObjectPtr<AActor> LevitatedTarget;

	// 취소/종료 시 제거할 Gameplay Effect 핸들
	FActiveGameplayEffectHandle ActiveLevitationHandle;

	// 타이머를 관리하기 위한 핸들
	FTimerHandle LevitationTimerHandle;

protected:
	void UpdateHovering(); // <- 부양(Hovering) 높이 유지용 업데이트 함수 추가

	// ====== 런타임 상태 ====== 추가
	UPROPERTY(Transient)
	FTimerHandle HoverTimerHandle;

	UPROPERTY(Transient)
	float HoverTargetZ = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UPrimitiveComponent> HoverTargetComp;

	UPROPERTY(Transient)
	float HoverInitialZ = 0.f;

	UPROPERTY(Transient)
	float HoverElapsedTime = 0.f;

	UPROPERTY(Transient)
	float CurrentLevitateDuration = 0.f;
	
protected:
	virtual void OnPreCastFacingFinished(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
) override;

	void ExecuteLeviosaCast(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	);
};