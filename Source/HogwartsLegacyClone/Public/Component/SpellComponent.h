// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Core/HOG_Enum.h"
#include "Core/HOG_Struct.h"
#include "SpellComponent.generated.h"

class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnSpellCooldownStarted,
	FGameplayTag, SpellID,
	float, CooldownSeconds
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnSpellCooldownEnded,
	FGameplayTag, SpellID
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnSpellCastSucceeded,
	FGameplayTag, SpellID,
	ESpellCastContext, CastContext
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnSpellCastFailed,
	FGameplayTag, SpellID,
	ESpellCastFailReason, FailReason
);

/**
 * 일반 스펠 런타임 관리자
 * - 기본공격 제외
 * - 스펠 쿨타임 관리
 * - 스펠 사용 가능 여부 검사
 * - 패링/특수 발동 시 쿨타임 무시 정책 지원
 *
 * 추천 부착 위치:
 * - PlayerState
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HOGWARTSLEGACYCLONE_API USpellComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpellComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

public:
	/**
	 * 요청 기반 시전 가능 여부 검사
	 * - SpellID 유효성
	 * - Owner 유효성
	 * - 전체 Spell 잠금 여부
	 * - 상태 차단 태그 검사
	 * - 쿨타임 검사
	 */
	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	FSpellCastCheckResult CanCastSpell(const FSpellCastRequest& CastRequest) const;

	/**
	 * 실제 스펠 시전 성공 후 호출
	 * 정책:
	 * - Normal: 쿨타임 시작
	 * - ParryCounter: 기본적으로 쿨타임 시작 안 함
	 * - SpecialFreeCast: 기본적으로 쿨타임 시작 안 함
	 *
	 * 단, CastRequest의 bForceStartCooldown 이 true면 강제로 시작 가능
	 */
	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	void NotifySpellCastSuccess(const FSpellCastRequest& CastRequest);

	/**
	 * 시전 실패 브로드캐스트
	 */
	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	void NotifySpellCastFailed(
		FGameplayTag SpellID,
		ESpellCastFailReason FailReason
	);

	/**
	 * 쿨타임 수동 시작
	 */
	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	void StartCooldown(
		FGameplayTag SpellID,
		float CooldownSeconds
	);

	/**
	 * 쿨타임 수동 제거
	 */
	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	void ClearCooldown(FGameplayTag SpellID);

	/**
	 * 해당 스펠이 쿨타임 중인지
	 */
	UFUNCTION(BlueprintPure, Category="HOG|Spell")
	bool IsSpellOnCooldown(FGameplayTag SpellID) const;

	/**
	 * 해당 스펠이 준비 상태인지
	 */
	UFUNCTION(BlueprintPure, Category="HOG|Spell")
	bool IsSpellReady(FGameplayTag SpellID) const;

	/**
	 * 남은 쿨타임 조회
	 */
	UFUNCTION(BlueprintPure, Category="HOG|Spell")
	float GetRemainingCooldown(FGameplayTag SpellID) const;

	/**
	 * 총 쿨타임 조회
	 */
	UFUNCTION(BlueprintPure, Category="HOG|Spell")
	float GetTotalCooldown(FGameplayTag SpellID) const;

	/**
	 * 전체 스펠 사용 잠금
	 */
	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	void SetSpellCastingLocked(bool bLocked);

	/**
	 * 전체 스펠 사용 잠금 여부
	 */
	UFUNCTION(BlueprintPure, Category="HOG|Spell")
	bool IsSpellCastingLocked() const { return bSpellCastingLocked; }

	/**
	 * 쿨타임 맵 전체 복사
	 * - 디버그용
	 */
	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	void GetActiveCooldownMap(TMap<FGameplayTag, float>& OutCooldownMap) const;

	/**
	 * UI/디버그용 배열 스냅샷
	 */
	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	void GetActiveCooldownEntries(TArray<FSpellCooldownEntry>& OutEntries) const;

	/**
	 * 현재 컨텍스트가 쿨타임 체크를 무시하는지
	 */
	UFUNCTION(BlueprintPure, Category="HOG|Spell")
	bool ShouldIgnoreCooldownCheck(const FSpellCastRequest& CastRequest) const;

	/**
	 * 현재 컨텍스트가 시전 성공 후 쿨타임을 시작하는지
	 */
	UFUNCTION(BlueprintPure, Category="HOG|Spell")
	bool ShouldStartCooldownAfterCast(const FSpellCastRequest& CastRequest) const;

public:
	UPROPERTY(BlueprintAssignable, Category="HOG|Spell")
	FOnSpellCooldownStarted OnSpellCooldownStarted;

	UPROPERTY(BlueprintAssignable, Category="HOG|Spell")
	FOnSpellCooldownEnded OnSpellCooldownEnded;

	UPROPERTY(BlueprintAssignable, Category="HOG|Spell")
	FOnSpellCastSucceeded OnSpellCastSucceeded;

	UPROPERTY(BlueprintAssignable, Category="HOG|Spell")
	FOnSpellCastFailed OnSpellCastFailed;

protected:
	/**
	 * Owner ASC에서 차단 태그 검사
	 * 기본 검사 대상:
	 * - BlockingOwnerTags
	 *
	 * CastRequest.bIgnoreStateBlock == true 면 스킵
	 */
	bool FindBlockingOwnerTag(
		const FSpellCastRequest& CastRequest,
		FGameplayTag& OutBlockingTag
	) const;

	/**
	 * Owner에서 ASC 찾기
	 * 우선순위:
	 * 1) IAbilitySystemInterface
	 * 2) FindComponentByClass<UAbilitySystemComponent>()
	 */
	UAbilitySystemComponent* GetOwnerASC() const;

protected:
	/**
	 * SpellID -> Remaining Cooldown Seconds
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="HOG|Spell")
	TMap<FGameplayTag, float> ActiveCooldownMap;

	/**
	 * SpellID -> Total Cooldown Seconds
	 * UI 비율 계산용
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="HOG|Spell")
	TMap<FGameplayTag, float> TotalCooldownMap;

	/**
	 * 전체 스펠 사용 잠금
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HOG|Spell")
	bool bSpellCastingLocked = false;

	/**
	 * 일반 시전 차단 태그
	 * 예:
	 * - State.Dead
	 * - State.Debuff.Stunned
	 * - State.Debuff.Silenced
	 * - State.Spell.Locked
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HOG|Spell")
	FGameplayTagContainer BlockingOwnerTags;
};
