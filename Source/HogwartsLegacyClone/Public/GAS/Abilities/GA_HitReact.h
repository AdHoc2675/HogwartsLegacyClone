#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_HitReact.generated.h"

class UAnimMontage;

/**
 * 피격 반응 전용 GameplayAbility
 * - CombatComponent 에서 State.Hit 태그로 TryActivateAbilitiesByTag 호출 시 발동
 * - 지정된 피격 몽타주를 1회 재생하고 즉시 종료
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UGA_HitReact : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_HitReact();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|HitReact")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|HitReact")
	float PlayRate = 1.0f;
};