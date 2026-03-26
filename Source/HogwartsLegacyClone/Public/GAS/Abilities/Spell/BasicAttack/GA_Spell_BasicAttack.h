// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/GA_SpellBase.h"
#include "GA_Spell_BasicAttack.generated.h"

class UAnimMontage;

UCLASS()
class HOGWARTSLEGACYCLONE_API UGA_Spell_BasicAttack : public UGA_SpellBase
{
	GENERATED_BODY()
	
public:
	UGA_Spell_BasicAttack();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void InputPressed(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
	) override;

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

public:
	UFUNCTION()
	void TryAdvanceComboFromBranchPoint();

protected:
	// 0 = 1타, 1 = 2타, 2 = 3타, 3 = 4타
	UPROPERTY(EditDefaultsOnly, Category="HOG|Spell|BasicAttack|Anim")
	TArray<TObjectPtr<UAnimMontage>> ComboMontages;

	UPROPERTY(Transient)
	int32 ComboIndex = 0;

	UPROPERTY(Transient)
	bool bComboInProgress = false;

	UPROPERTY(Transient)
	bool bNextComboQueued = false;

	UPROPERTY(Transient)
	int32 CurrentComboStep = 0;
	
	// 콤보 입력 버퍼 유지 시간(초)
	UPROPERTY(EditDefaultsOnly, Category="HOG|Spell|BasicAttack|Combo")
	float ComboInputBufferSeconds = 0.3f;

	// 마지막 콤보 입력 예약 시각
	UPROPERTY(Transient)
	float LastComboQueuedTime = -1.f;

	// 현재 타수에서 Branch Notify를 이미 소비했는지
	UPROPERTY(Transient)
	bool bBranchConsumed = false;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> CurrentPlayingMontage = nullptr;

	// Branch Notify로 의도적으로 다음 타로 전환 중인지
	UPROPERTY(Transient)
	bool bAdvancingComboFromBranch = false;

	// 같은 타수에서 투사체가 이미 발사되었는지
	UPROPERTY(Transient)
	bool bProjectileFiredThisStep = false;

	UPROPERTY(EditDefaultsOnly, Category="HOG|Spell|BasicAttack|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, Category="HOG|Spell|BasicAttack|Trace")
	bool bIgnoreSelf = true;

	UPROPERTY(EditDefaultsOnly, Category="HOG|Spell|BasicAttack|Debug")
	bool bDrawDebugLine = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BasicAttack|Projectile")
	TSubclassOf<class ABasicAttackActor> BasicAttackActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BasicAttack|Projectile")
	float ProjectileDamage = 10.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BasicAttack|Projectile")
	float FourthComboProjectileDamage = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BasicAttack|Projectile")
	float ProjectileSpawnForwardOffset = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="BasicAttack|Projectile")
	float ProjectileSpawnUpOffset = 20.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HOG|Spell|BasicAttack|Sound")
	TObjectPtr<class USoundBase> CastSound = nullptr;

private:
	void PlayComboMontageOrFire(const FGameplayAbilityActorInfo* ActorInfo);

	bool TryPlayCurrentComboMontage(const FGameplayAbilityActorInfo* ActorInfo);

	void FireHitScan(const FGameplayAbilityActorInfo* ActorInfo);
	
	bool IsComboInputBufferedValid() const;

	bool BuildTraceStartEnd(
		const FGameplayAbilityActorInfo* ActorInfo,
		FVector& OutStart,
		FVector& OutEnd,
		AActor*& OutLockTarget
	) const;

	void ResetComboState();
	
public:
	void SpawnBasicAttackActor();
	
protected:
	virtual void OnPreCastFacingFinished(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	void ExecuteBasicAttackCast(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	);
};