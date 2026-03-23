#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "Core/HOG_Enum.h"
#include "HOG_Struct.generated.h"

class UNiagaraSystem;

USTRUCT(BlueprintType)
struct HOGWARTSLEGACYCLONE_API FDamageRequest
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	TObjectPtr<AActor> DamageCauser = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	float BaseDamage = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	FGameplayTag DamageTypeTag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	FHitResult HitResult;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	FGameplayTagContainer SourceTags;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Damage")
	FGameplayTagContainer TargetTags;
};

USTRUCT(BlueprintType)
struct HOGWARTSLEGACYCLONE_API FDamageResult
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Damage")
	bool bWasApplied = false;

	UPROPERTY(BlueprintReadOnly, Category="Damage")
	bool bWasBlocked = false;

	UPROPERTY(BlueprintReadOnly, Category="Damage")
	bool bWasParried = false;

	UPROPERTY(BlueprintReadOnly, Category="Damage")
	bool bKilledTarget = false;

	UPROPERTY(BlueprintReadOnly, Category="Damage")
	float FinalDamage = 0.0f;
};

/**
 * SpellComponent에 시전 요청을 넘길 때 사용하는 런타임 요청 데이터
 * - 일반 발동 / 패링 반격 / 특수 무료 발동 구분
 * - SpellID와 쿨타임 값을 함께 전달 가능
 */
USTRUCT(BlueprintType)
struct HOGWARTSLEGACYCLONE_API FSpellCastRequest
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spell")
	FGameplayTag SpellID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spell")
	ESpellCastContext CastContext = ESpellCastContext::Normal;

	/**
	 * Definition에서 읽어온 기본 쿨타임 값
	 * SpellComponent의 NotifySpellCastSuccess 쪽에서 사용 가능
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spell")
	float CooldownSeconds = 0.0f;

	/**
	 * true면 상태 이상/잠금까지 전부 무시하는 강제 발동용 확장 플래그
	 * 지금 1차 구현에서는 기본 false 유지 추천
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spell")
	bool bIgnoreStateBlock = false;

	/**
	 * true면 시전 성공 후 쿨타임을 강제로 시작
	 * 보통 Normal은 true, ParryCounter는 false 정책으로 감
	 * 필요 시 특수 상황에서 덮어쓰기 가능
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spell")
	bool bForceStartCooldown = false;

	/**
	 * true면 시전 전에 쿨타임 체크를 강제로 무시
	 * ParryCounter / SpecialFreeCast 같은 특수 발동 지원용
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spell")
	bool bForceIgnoreCooldownCheck = false;
};

/**
 * SpellComponent 내부에서 관리할 스펠별 쿨타임 엔트리
 */
USTRUCT(BlueprintType)
struct HOGWARTSLEGACYCLONE_API FSpellCooldownEntry
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spell")
	FGameplayTag SpellID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spell")
	float RemainingTime = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Spell")
	float TotalTime = 0.0f;

	FSpellCooldownEntry()
	{
	}

	FSpellCooldownEntry(const FGameplayTag& InSpellID, float InRemainingTime, float InTotalTime)
		: SpellID(InSpellID)
		  , RemainingTime(InRemainingTime)
		  , TotalTime(InTotalTime)
	{
	}
};

/**
 * SpellComponent의 시전 가능 여부 검사 결과
 * 문자열 대신 enum + 태그로 실패 원인을 구조화해서 전달
 */
USTRUCT(BlueprintType)
struct HOGWARTSLEGACYCLONE_API FSpellCastCheckResult
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Spell")
	bool bCanCast = true;

	UPROPERTY(BlueprintReadOnly, Category="Spell")
	ESpellCastFailReason FailReason = ESpellCastFailReason::None;

	/**
	 * 실패 원인이 특정 상태 태그일 경우 저장
	 * 예: State.Dead / State.Debuff.Stunned / State.Debuff.Silenced
	 */
	UPROPERTY(BlueprintReadOnly, Category="Spell")
	FGameplayTag BlockingTag;
};


USTRUCT(BlueprintType)
struct HOGWARTSLEGACYCLONE_API FQueuedSpellVFXData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPending = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UNiagaraSystem> NiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName StartSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BeamStartParameterName = TEXT("BeamStart");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BeamEndParameterName = TEXT("BeamEnd");

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BeamLengthParameterName = TEXT("BeamLength");
};