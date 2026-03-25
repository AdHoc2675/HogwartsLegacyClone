#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/GA_SpellBase.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "GA_Spell_Protego.generated.h"


class UGE_Protego;
class AProtegoActor;
class UCombatComponent;
class UGameplayAbility;
class UAbilitySystemComponent;
class USoundBase;
class UAnimMontage;

/**
 * Protego 스펠 Ability
 * - 자신에게 GE_Protego 적용
 * - ProtegoActor 생성 및 Avatar에 부착
 * - CombatComponent의 Protego Parry Window 오픈
 * - 패링 성공 시 자동 Stupefy 반격
 * - 종료 시 보호막 Actor / GE / 바인딩 정리
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UGA_Spell_Protego : public UGA_SpellBase
{
	GENERATED_BODY()

public:
	UGA_Spell_Protego();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	UFUNCTION()
	void HandleParrySuccess(AActor* AttackerActor);

protected:
	UCombatComponent* GetOwnerCombatComponent() const;
	bool TryTriggerCounterStupefy(AActor* AttackerActor);
	void BindCombatDelegates();
	void UnbindCombatDelegates();

protected:
	/** Protego 활성 상태를 부여하는 GE 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Protego")
	TSubclassOf<UGE_Protego> ProtegoEffectClass;

	/** 시각/충돌 담당 Protego Actor 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Protego")
	TSubclassOf<AProtegoActor> ProtegoActorClass;

	/** 패링 성공 시 자동 반격으로 사용할 Stupefy Ability 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Protego|Parry")
	TSubclassOf<UGameplayAbility> CounterStupefyAbilityClass;

	/** Protego 활성 직후 패링 판정 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Protego|Parry", meta=(ClampMin="0.01"))
	float ParryWindowSeconds = 0.30f;

	/** Protego 활성 직후 Block 판정 시간 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Protego|Timing", meta=(ClampMin="0.01"))
	float BlockWindowSeconds = 1.00f;

	/** 시전 보이스 사운드 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Protego|Sound")
	TObjectPtr<USoundBase> CastVoiceSound = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "HOG|Spell|Protego|Sound")
	FString CastSubtitleText = TEXT("프로테고!");

	/** 시전 일반 사운드 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Protego|Sound")
	TObjectPtr<USoundBase> CastSound = nullptr;

	/** 패링 성공 사운드 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Protego|Sound")
	TObjectPtr<USoundBase> ParrySuccessSound = nullptr;

	/** Protego 발동 애니메이션 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Protego|Anim")
	TObjectPtr<UAnimMontage> CastMontage = nullptr;

	/** 스폰된 Protego Actor 참조 */
	UPROPERTY()
	TObjectPtr<AProtegoActor> SpawnedProtegoActor;

	/** 적용한 GE 핸들 */
	FActiveGameplayEffectHandle ActiveProtegoEffectHandle;

	/** 바인딩된 CombatComponent 캐시 */
	UPROPERTY(Transient)
	TObjectPtr<UCombatComponent> CachedCombatComponent;

	bool CanTriggerCounterStupefyFromAttacker(AActor* AttackerActor) const;

	UAbilitySystemComponent* ResolveAbilitySystemComponentFromActor(AActor* InActor) const;

	FTimerHandle ProtegoLifetimeTimerHandle;

protected:
	virtual bool ShouldApplyCastingActiveTag() const override;
};
