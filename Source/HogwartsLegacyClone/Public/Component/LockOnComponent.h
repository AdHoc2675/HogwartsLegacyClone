// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LockOnComponent.generated.h"

// 락온된 대상이 변경이 된 경우
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLockOnTargetChanged, const FLockOnTargetResult&);

class UAbilitySystemComponent;
class UPrimitiveComponent;

USTRUCT(BlueprintType)
struct FLockOnTargetResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTagContainer TargetTags;

	UPROPERTY(BlueprintReadOnly)
	FVector AimPoint = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	float Distance = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float AngleDegrees = 0.f;

	UPROPERTY(BlueprintReadOnly)
	float Score = -1.f;
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HOGWARTSLEGACYCLONE_API ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULockOnComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

public:
	// ===== 기존 API =====
	// 외부에서 즉시 "최적 타겟 계산"이 필요할 때 사용 가능
	UFUNCTION(BlueprintCallable, Category="HOG|LockOn")
	bool FindBestTarget(
		const FGameplayTagContainer& RequiredTargetTags,
		FLockOnTargetResult& OutResult
	) const;

	// 타겟이 없을 때도 AimPoint는 필요해서 분리 제공(히트스캔 fallback용)
	UFUNCTION(BlueprintCallable, Category="HOG|LockOn")
	bool GetCenterAimPoint(FVector& OutAimPoint) const;

public:
	// ===== 현재 타겟 상태 API =====
	UFUNCTION(BlueprintPure, Category="HOG|LockOn")
	bool HasValidCurrentTarget() const;

	UFUNCTION(BlueprintCallable, Category="HOG|LockOn")
	bool TryGetLockedTargetResult(FLockOnTargetResult& OutResult) const;

	UFUNCTION(BlueprintPure, Category="HOG|LockOn")
	AActor* GetCurrentTarget() const { return CurrentTargetActor.Get(); }

	UFUNCTION(BlueprintPure, Category="HOG|LockOn")
	const FLockOnTargetResult& GetCurrentTargetResult() const { return CurrentTargetResult; }

	UFUNCTION(BlueprintCallable, Category="HOG|LockOn")
	void ForceRefreshTarget();

	UFUNCTION(BlueprintCallable, Category="HOG|LockOn")
	void ClearCurrentTarget();

public:
	// ===== 튜닝 값 =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Tuning")
	float MaxRange = 4000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Tuning")
	float MaxAngleDegrees = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Tuning")
	float AngleWeight = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Tuning")
	float DistanceWeight = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Tuning")
	bool bRequireLineOfSight = true;

public:
	// ===== 실시간 갱신 =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Realtime")
	bool bEnableRealtimeTargeting = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Realtime")
	bool bRealtimeOnlyWhenLocallyControlled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Realtime", meta=(ClampMin="0.01"))
	float RealtimeUpdateInterval = 0.05f;

public:
	// ===== 후보 수집 =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Filter")
	bool bIncludePawnTargets = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Filter")
	bool bIncludeWorldDynamicTargets = true;

	// 기본 후보 허용 태그
	// 예: Team_Enemy, Team_Object
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Filter")
	FGameplayTagContainer AllowedTargetTags;

public:
	// ===== 외곽선 =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Outline")
	bool bUseTargetOutline = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Outline", meta=(ClampMin="0", ClampMax="255"))
	int32 EnemyOutlineStencilValue = 111;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Outline", meta=(ClampMin="0", ClampMax="255"))
	int32 ObjectOutlineStencilValue = 112;

public:
	// ===== 디버그 =====
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Debug")
	bool bDebugPrint = false;
	
	// ===== 락온 대상 변경시 =====
public:
	FOnLockOnTargetChanged OnLockOnTarget;
	FOnLockOnTargetChanged OnLockOnReleased;

private:
	bool ShouldRunRealtimeTargeting() const;

	bool GetCameraView(FVector& OutCamLoc, FVector& OutCamForward) const;

	bool HasLineOfSight(const FVector& CamLoc, AActor* Candidate) const;

	float ComputeScore(float AngleDeg, float Dist) const;

private:
	// ===== 실시간 갱신 내부 =====
	void RefreshCurrentTarget();

	bool FindBestTargetInternal(
		const FGameplayTagContainer& RequiredTargetTags,
		FLockOnTargetResult& OutResult
	) const;

	void GatherCandidateActors(TArray<AActor*>& OutCandidates) const;

	bool EvaluateCandidate(
		AActor* Candidate,
		const FVector& CamLoc,
		const FVector& CamForward,
		const FGameplayTagContainer& RequiredTargetTags,
		FLockOnTargetResult& OutCandidateResult
	) const;

	bool IsTargetCandidate(
		AActor* Candidate,
		const FGameplayTagContainer& RequiredTargetTags,
		FGameplayTagContainer& OutCandidateTags
	) const;

private:
	// ===== 현재 상태 =====
	void SetCurrentTarget(AActor* NewTarget, const FLockOnTargetResult& NewResult);

	void ApplyTargetOutline(AActor* TargetActor, const FGameplayTagContainer& TargetTags, bool bEnable) const;

	void ResolveTargetPrimitiveComponents(
		AActor* TargetActor,
		TArray<UPrimitiveComponent*>& OutComponents
	) const;

	int32 GetOutlineStencilValueForTarget(const FGameplayTagContainer& TargetTags) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentTargetActor = nullptr;

	UPROPERTY(Transient)
	FLockOnTargetResult CurrentTargetResult;

	UPROPERTY(Transient)
	bool bHasValidTarget = false;

	UPROPERTY(Transient)
	float TimeSinceLastRefresh = 0.f;
};
