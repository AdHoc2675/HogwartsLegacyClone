#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GAS/Abilities/GA_Base.h"
#include "Core/HOG_Enum.h"
#include "Core/HOG_Struct.h"
#include "GA_SpellBase.generated.h"

class UDA_SpellDefinition;
class APlayerCharacterBase;
class ULockOnComponent;
class USpellComponent;
class UNiagaraSystem;

/**
 * UGA_SpellBase
 *
 * [역할]
 * - “모든 스펠(마법) Ability”의 공통 베이스 클래스.
 * - 이 클래스는 스펠의 실제 효과(Accio 당기기, Stupefy 충격 등)를 구현하지 않는다.
 * - 대신 “스펠 데이터(쿨타임/데미지/사거리/타겟 조건)”를 DataAsset로부터 가져오고,
 *   공통 검증(타겟 태그 조건 등)을 제공한다.
 *
 * [데이터 파이프라인]
 *   (1) UDA_SpellDefinition (PrimaryDataAsset)
 *       - SpellID(게임플레이태그)를 키로, 쿨타임/데미지/사거리/타겟 요구태그 등을 보관
 *   (2) UHOG_GameInstance 의 SpellRegistry
 *       - TMap<SpellID, Definition> 형태로 런타임 조회 가능
 *   (3) UGA_SpellBase (이 클래스)
 *       - SpellID를 가지고 있다가, 실행 시 GameInstance에서 Definition을 조회하여 사용
 *
 * [중요 설계 의도]
 * - 스펠의 “진실의 원천”은 Definition(DataAsset)이다.
 * - 스펠 Ability는 가능한 한 하드코딩을 줄이고, Definition을 읽어서 동작하도록 한다.
 * - 이후 “스펠 매핑(SpellID -> Primary/Alt GAClass)”은 별도 Mapping DataAsset에서 관리 예정.
 *
 * [추가된 역할]
 * - PlayerState의 SpellComponent 접근
 * - 일반 시전 / 패링 반격 / 특수 무료 시전용 공통 시전 요청 빌드
 * - 공통 쿨타임 체크 / 성공 / 실패 통지
 */
UCLASS(Abstract)
class HOGWARTSLEGACYCLONE_API UGA_SpellBase : public UGA_Base
{
	GENERATED_BODY()

public:
	UGA_SpellBase();

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Spell")
	FGameplayTag SpellID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Spell|Debug")
	bool bWarnIfDefinitionMissing = true;

public:
	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	UDA_SpellDefinition* GetSpellDefinition() const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	UDA_SpellDefinition* GetSpellDefinitionOrWarn() const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	float GetCooldownSeconds() const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	float GetBaseDamage() const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell")
	float GetCastRange() const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell|Targeting")
	bool DoesTargetMeetRequirements(AActor* Target) const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell|Targeting")
	bool TryConsumeLockedTarget(AActor*& OutTarget, FGameplayTagContainer& OutTargetTags, FVector& OutAimPoint) const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell|Targeting")
	bool BuildFallbackAimPoint(FVector& OutAimPoint, float RangeOverride = -1.f) const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell|Facing")
	bool GetCachedPreCastFacingTargetLocation(FVector& OutTargetLocation) const;

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
	UFUNCTION(BlueprintCallable, Category="HOG|Spell|Runtime")
	USpellComponent* GetSpellComponent() const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell|Runtime")
	virtual FSpellCastRequest BuildSpellCastRequest(ESpellCastContext CastContext) const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell|Runtime")
	FSpellCastCheckResult CheckCanCastSpell(ESpellCastContext CastContext) const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell|Runtime")
	void NotifySpellCastFailedResult(
		const FSpellCastRequest& CastRequest,
		const FSpellCastCheckResult& CheckResult
	) const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell|Runtime")
	void NotifySpellCastSucceeded(ESpellCastContext CastContext) const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell|Runtime")
	bool CanCastAsNormal(FSpellCastCheckResult& OutCheckResult) const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell|Runtime")
	bool CanCastAsParryCounter(FSpellCastCheckResult& OutCheckResult) const;

	UFUNCTION(BlueprintCallable, Category="HOG|Spell|Runtime")
	bool CanCastAsSpecialFreeCast(FSpellCastCheckResult& OutCheckResult) const;

protected:
	bool IsTargetBlocked(AActor* Target, const FGameplayTagContainer& Blocked) const;
	bool HasAllRequiredTags(AActor* Target, const FGameplayTagContainer& Required) const;

protected:
	virtual bool ShouldApplyCastingActiveTag() const;

protected:
	// =========================
	// Pre-Cast Facing
	// =========================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Spell|Facing")
	bool bRequireFacingBeforeCast = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Spell|Facing")
	float PreCastFacingPollInterval = 0.01f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="HOG|Spell|Facing")
	float PreCastFacingTimeout = 0.20f;

	UPROPERTY(Transient)
	bool bWaitingForPreCastFacing = false;

	UPROPERTY(Transient)
	float PreCastFacingElapsed = 0.f;

	UPROPERTY(Transient)
	FTimerHandle PreCastFacingTimerHandle;

	UPROPERTY(Transient)
	FGameplayAbilitySpecHandle CachedFacingHandle;

	UPROPERTY(Transient)
	FGameplayAbilityActivationInfo CachedFacingActivationInfo;

	UPROPERTY(Transient)
	TObjectPtr<const UGameplayAbility> CachedFacingAbilityForSafety = nullptr;

	UPROPERTY(Transient)
	FVector CachedPreCastFacingTargetLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	bool bHasCachedPreCastFacingTargetLocation = false;

protected:
	virtual bool TryBuildPreCastFacingTargetLocation(FVector& OutTargetLocation) const;

	bool TryBeginPreCastFacing(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	);

	void TickPreCastFacing();

	virtual void OnPreCastFacingFinished(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	);

	virtual bool ShouldDeferCastUntilFacingFinished() const;

protected:
	// =========================
	// LineTrace Beam VFX Queue
	// =========================
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> QueuedLineTraceBeamVFX = nullptr;

	UPROPERTY(Transient)
	FVector QueuedLineTraceBeamTargetLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	FName QueuedBeamStartSocketName = NAME_None;

	UPROPERTY(Transient)
	FName QueuedBeamStartParamName = TEXT("BeamStart");

	UPROPERTY(Transient)
	FName QueuedBeamEndParamName = TEXT("BeamEnd");

	UPROPERTY(Transient)
	FName QueuedBeamLengthParamName = TEXT("BeamLength");

	void QueueLineTraceSpellVFX(
	UNiagaraSystem* InVFX,
	const FVector& InTargetLocation,
	FName InStartSocketName = TEXT("RightHandWandSocket"),
	FName InBeamStartParam = TEXT("BeamStart"),
	FName InBeamEndParam = TEXT("BeamEnd"),
	FName InBeamLengthParam = TEXT("BeamLength")
);

	void ClearQueuedLineTraceSpellVFX();

	bool SpawnQueuedLineTraceSpellVFX();

	void RegisterCastNotifyToOwner();

public:
	virtual void HandleCastNotify();
};