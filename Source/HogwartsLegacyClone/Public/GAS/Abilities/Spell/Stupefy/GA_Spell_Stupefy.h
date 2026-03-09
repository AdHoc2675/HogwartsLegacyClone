#pragma once

#include "CoreMinimal.h"
#include "Core/HOG_Enum.h"
#include "GAS/Abilities/GA_SpellBase.h"
#include "GA_Spell_Stupefy.generated.h"

class UGameplayEffect;
class AActor;

/**
 * Stupefy
 * - 일반 시전: LockOn 타겟 대상 즉시 명중, 데미지만 적용
 * - 패링 반격: ForcedTarget 대상 즉시 명중, 데미지 + 기절 적용
 *
 * 슬로우 모션은 Ability에서 직접 처리하지 않고,
 * ANS_Stupefy_Slowmotion 에서 처리한다.
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UGA_SpellStupefy : public UGA_SpellBase
{
	GENERATED_BODY()

public:
	UGA_SpellStupefy();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

public:
	/**
	 * 다음 1회 발동용 CastContext / ForcedTarget 설정
	 * - 일반 입력 발동은 기본 Normal
	 * - Protego 패링 성공 시 ParryCounter + 공격자 전달
	 */
	UFUNCTION(BlueprintCallable, Category="HOG|Spell|Stupefy")
	void PrepareCastContext(
		ESpellCastContext InCastContext,
		AActor* InForcedTarget
	);

protected:
	/**
	 * 이번 발동의 실제 CastContext를 꺼내고 소비
	 */
	ESpellCastContext ResolveCastContextAndConsume();

	/**
	 * 이번 발동의 타겟 결정
	 * - Normal: LockOn 기반
	 * - ParryCounter: ForcedTarget 우선, 없으면 LockOn fallback
	 */
	bool ResolveTargetForCast(
		ESpellCastContext InCastContext,
		AActor*& OutTarget,
		FGameplayTagContainer& OutTargetTags,
		FVector& OutAimPoint
	);

	/**
	 * 실제 스투페파이 적용
	 * - Normal: 데미지만
	 * - ParryCounter: 데미지 + 기절
	 */
	bool ApplyStupefyToTarget(
		ESpellCastContext InCastContext,
		AActor* TargetActor,
		const FGameplayTagContainer& TargetTags
	);

	/**
	 * Target ASC에 기절 GE 적용
	 * - 패링 반격 시에만 호출
	 */
	bool ApplyStunEffectToTarget(AActor* TargetActor);

	/**
	 * 사용 후 1회성 pending 값 정리
	 */
	void ResetPendingCastData();

protected:
	/**
	 * 패링 반격 Stupefy 명중 시 대상에게 적용할 기절 GE
	 * BP에서 BP_GE_Stun_Stupefy 같은 식으로 연결
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Spell|Stupefy")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	/**
	 * 패링 반격 연출용 ANS 이름 메모용
	 * 실제 참조/실행은 애니메이션 쪽에서 처리
	 * 나중에 애니메이션에 ANS_Stupefy_Slowmotion 만든다음 이걸로 slowmotion 주면 됨
	 * 
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Spell|Stupefy|Anim")
	FName ParrySlowMotionANSName = TEXT("ANS_Stupefy_Slowmotion");

	/**
	 * 다음 1회 발동용 CastContext
	 */
	UPROPERTY(Transient)
	ESpellCastContext PendingCastContext = ESpellCastContext::Normal;

	/**
	 * 다음 1회 발동용 강제 대상
	 */
	UPROPERTY(Transient)
	TObjectPtr<AActor> PendingForcedTarget = nullptr;
};