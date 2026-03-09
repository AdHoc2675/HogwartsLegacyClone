// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/GA_SpellBase.h"
#include "GA_Spell_Incendio.generated.h"

class UAnimMontage;
class UGameplayEffect;
class UNiagaraSystem;

/**
 * 근거리 화염 마법 (Incendio)
 * - 전방 부채꼴(또는 구형 트레이스) 범위 내의 적들에게 즉발 데미지
 * - 명중한 적에게 화상(Damage over Time) 지속 효과(GE) 부여
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UGA_Spell_Incendio : public UGA_SpellBase
{
	GENERATED_BODY()

public:
	UGA_Spell_Incendio();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	// 애니메이션 종료 콜백
	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 실제 화염 발사 처리 (애니메이션 노티파이에서 호출하거나 몽타주 시작 시 즉시 호출)
	UFUNCTION(BlueprintCallable, Category = "HOG|Spell|Incendio")
	void FireIncendio();

protected:
	// ====== Incendio 옵션 ======

	// 캐스팅 몽타주
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Incendio|Anim")
	TObjectPtr<UAnimMontage> CastMontage;

	// 화염 범위 (반경). 사거리(길이)는 Definition의 CastRange를 사용
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Incendio|Trace")
	float AttackRadius = 150.f;

	// 타격 대상 콜리전 채널
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Incendio|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// ====== 효과 (Gameplay Effects) ======

	// 즉발 데미지 이펙트 (폭발 딜)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HOG|Spell|Incendio|Effect")
	TSubclassOf<UGameplayEffect> InstantDamageEffectClass;

	// 지속 화상 데미지 이펙트 (DoT - 상태이상)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HOG|Spell|Incendio|Effect")
	TSubclassOf<UGameplayEffect> DotDamageEffectClass;

	// ====== 시각 효과 ======
	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Incendio|Visual")
	TObjectPtr<UNiagaraSystem> FireVFX;

	UPROPERTY(EditDefaultsOnly, Category = "HOG|Spell|Incendio|Debug")
	bool bDrawDebug = false;
};