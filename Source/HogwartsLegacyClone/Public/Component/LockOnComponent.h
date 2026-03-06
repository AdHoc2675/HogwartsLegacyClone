// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include  "GameplayTagContainer.h"
#include "LockOnComponent.generated.h"

class UAbilitySystemComponent;

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




UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class HOGWARTSLEGACYCLONE_API ULockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULockOnComponent();

	// RequiredTargetTags: 타겟이 반드시 가지고 있어야 하는 태그(ASC Owned Tags 기준)
	// OutResult: 최종 타겟/태그/점수 등 결과
	UFUNCTION(BlueprintCallable, Category="HOG|LockOn")
	bool FindBestTarget(
		const FGameplayTagContainer& RequiredTargetTags,
		FLockOnTargetResult& OutResult
	) const;

	// 타겟이 없을 때도 AimPoint는 필요해서 분리 제공(히트스캔 fallback에 사용)
	UFUNCTION(BlueprintCallable, Category="HOG|LockOn")
	bool GetCenterAimPoint(FVector& OutAimPoint) const;

public:
	// ===== 튜닝 값 =====
	// 탐색 반경
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Tuning")
	float MaxRange = 2000.f;

	// 시야 중심 허용 각도(도). 이 각도 밖은 후보에서 제외
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Tuning")
	float MaxAngleDegrees = 12.f;

	// 후보 점수 가중치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Tuning")
	float AngleWeight = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Tuning")
	float DistanceWeight = 0.3f;

	// 라인오브사이트(LOS) 체크 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Tuning")
	bool bRequireLineOfSight = true;

	// Overlap 쿼리에서 제외할 오브젝트 타입(기본: Pawn만 찾는 방식으로 cpp에서 설정)
	// 필요하면 확장

	// 디버그 로그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|LockOn|Debug")
	bool bDebugPrint = false;

private:
	bool GetCameraView(FVector& OutCamLoc, FVector& OutCamForward) const;

	bool PassRequiredTags(AActor* Candidate, const FGameplayTagContainer& RequiredTargetTags, FGameplayTagContainer& OutCandidateTags) const;

	bool HasLineOfSight(const FVector& CamLoc, AActor* Candidate) const;

	float ComputeScore(float AngleDeg, float Dist) const;

		
};
