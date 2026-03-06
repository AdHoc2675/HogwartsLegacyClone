// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/GA_SpellBase.h"
#include "GA_Spell_BasicAttack.generated.h"

class UAnimMontage;
/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UGA_Spell_BasicAttack : public UGA_SpellBase
{
	GENERATED_BODY()
	
public:
	UGA_Spell_BasicAttack();

protected:
	// Ability 실행
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	// 몽타주 끝나면 종료
	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

protected:
	// ====== BasicAttack 옵션 ======

	// 콤보 몽타주들(1~4개 등). 비어있으면 애니 없이 바로 발사.
	UPROPERTY(EditDefaultsOnly, Category="HOG|Spell|BasicAttack|Anim")
	TArray<TObjectPtr<UAnimMontage>> ComboMontages;

	// 콤보 인덱스 (런타임)
	UPROPERTY(Transient)
	int32 ComboIndex = 0;

	// 히트스캔 충돌 채널
	UPROPERTY(EditDefaultsOnly, Category="HOG|Spell|BasicAttack|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// 자기 자신/무기 등 제외할지
	UPROPERTY(EditDefaultsOnly, Category="HOG|Spell|BasicAttack|Trace")
	bool bIgnoreSelf = true;

	// 디버그 라인(선택)
	UPROPERTY(EditDefaultsOnly, Category="HOG|Spell|BasicAttack|Debug")
	bool bDrawDebugLine = false;

private:
	void PlayComboMontageOrFire(const FGameplayAbilityActorInfo* ActorInfo);

	void FireHitScan(const FGameplayAbilityActorInfo* ActorInfo);

	bool BuildTraceStartEnd(const FGameplayAbilityActorInfo* ActorInfo, FVector& OutStart, FVector& OutEnd, AActor*& OutLockTarget) const;
	
};
