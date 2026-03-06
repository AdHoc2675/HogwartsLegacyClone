// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/LockOnComponent.h"

#include "HOGDebugHelper.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"

#include "Kismet/KismetSystemLibrary.h"


ULockOnComponent::ULockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool ULockOnComponent::GetCameraView(FVector& OutCamLoc, FVector& OutCamForward) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return false;

	const APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC) return false;

	const APlayerCameraManager* CamMgr = PC->PlayerCameraManager;
	if (!CamMgr) return false;

	OutCamLoc = CamMgr->GetCameraLocation();
	OutCamForward = CamMgr->GetActorForwardVector(); 
	OutCamForward = OutCamForward.GetSafeNormal();
	return true;
}

bool ULockOnComponent::GetCenterAimPoint(FVector& OutAimPoint) const
{
	FVector CamLoc, CamForward;
	if (!GetCameraView(CamLoc, CamForward)) return false;

	OutAimPoint = CamLoc + (CamForward * MaxRange);
	return true;
}

float ULockOnComponent::ComputeScore(float AngleDeg, float Dist) const
{
	const float Angle01 = 1.f - FMath::Clamp(AngleDeg / FMath::Max(0.01f, MaxAngleDegrees), 0.f, 1.f);
	const float Dist01  = 1.f - FMath::Clamp(Dist / FMath::Max(1.f, MaxRange), 0.f, 1.f);
	return (Angle01 * AngleWeight) + (Dist01 * DistanceWeight);
}

bool ULockOnComponent::HasLineOfSight(const FVector& CamLoc, AActor* Candidate) const
{
	if (!bRequireLineOfSight) return true;
	if (!Candidate) return false;

	UWorld* World = GetWorld();
	if (!World) return false;

	const FVector TargetLoc = Candidate->GetActorLocation();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(HOG_LockOn_LOS), false);
	Params.AddIgnoredActor(GetOwner());

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		CamLoc,
		TargetLoc,
		ECC_Visibility,
		Params
	);

	// 아무것도 안 맞으면(=가림 없음) true
	if (!bHit) return true;

	// 첫 히트가 Candidate면 true
	return (Hit.GetActor() == Candidate);
}

bool ULockOnComponent::PassRequiredTags(AActor* Candidate, const FGameplayTagContainer& RequiredTargetTags,
	FGameplayTagContainer& OutCandidateTags) const
{
	OutCandidateTags.Reset();

	if (!Candidate) return false;

	// RequiredTargetTags가 비어있으면 “태그 조건 없음”
	if (RequiredTargetTags.IsEmpty())
	{
		// 그래도 태그는 가능하면 채워준다
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Candidate))
		{
			ASC->GetOwnedGameplayTags(OutCandidateTags);
		}
		return true;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Candidate);
	if (!ASC) return false;

	ASC->GetOwnedGameplayTags(OutCandidateTags);

	// RequiredTargetTags 모두 포함해야 통과(AND)
	return OutCandidateTags.HasAll(RequiredTargetTags);
}

bool ULockOnComponent::FindBestTarget(const FGameplayTagContainer& RequiredTargetTags,
	FLockOnTargetResult& OutResult) const
{
	 OutResult = FLockOnTargetResult();

    UWorld* World = GetWorld();
    if (!World) return false;

    FVector CamLoc, CamForward;
    if (!GetCameraView(CamLoc, CamForward)) return false;

    // 1) 후보 수집: Sphere Overlap (Pawn만)
    TArray<FOverlapResult> Overlaps;

    FCollisionObjectQueryParams ObjParams;
    ObjParams.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(HOG_LockOn_Overlap), false);
    QueryParams.AddIgnoredActor(GetOwner());

    const bool bAnyOverlap = World->OverlapMultiByObjectType(
        Overlaps,
        CamLoc,
        FQuat::Identity,
        ObjParams,
        FCollisionShape::MakeSphere(MaxRange),
        QueryParams
    );

    if (!bAnyOverlap || Overlaps.Num() == 0)
    {
        // 타겟 없음이어도 AimPoint는 채움
        GetCenterAimPoint(OutResult.AimPoint);
        return false;
    }

    float BestScore = -1.f;
    AActor* BestActor = nullptr;
    FGameplayTagContainer BestTags;
    float BestDist = 0.f;
    float BestAngle = 0.f;

    // 2) 후보 평가
    for (const FOverlapResult& O : Overlaps)
    {
        AActor* Candidate = O.GetActor();
        if (!Candidate) continue;
        if (Candidate == GetOwner()) continue;

        const FVector ToCandidate = (Candidate->GetActorLocation() - CamLoc);
        const float Dist = ToCandidate.Size();
        if (Dist <= KINDA_SMALL_NUMBER || Dist > MaxRange) continue;

        const FVector Dir = ToCandidate / Dist;
        const float Dot = FVector::DotProduct(CamForward, Dir);
        const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));

        // 시야 중심 각도 컷
        if (AngleDeg > MaxAngleDegrees) continue;

        // LOS 컷
        if (!HasLineOfSight(CamLoc, Candidate)) continue;

        // 태그 필터 컷 + Candidate 태그 가져오기
        FGameplayTagContainer CandidateTags;
        if (!PassRequiredTags(Candidate, RequiredTargetTags, CandidateTags)) continue;

        const float Score = ComputeScore(AngleDeg, Dist);

        if (Score > BestScore)
        {
            BestScore = Score;
            BestActor = Candidate;
            BestTags = CandidateTags;
            BestDist = Dist;
            BestAngle = AngleDeg;
        }
    }

    if (!BestActor)
    {
        GetCenterAimPoint(OutResult.AimPoint);
        return false;
    }

    OutResult.TargetActor = BestActor;
    OutResult.TargetTags = BestTags;
    OutResult.Distance = BestDist;
    OutResult.AngleDegrees = BestAngle;
    OutResult.Score = BestScore;
    OutResult.AimPoint = BestActor->GetActorLocation();

    if (bDebugPrint)
    {
        // 프로젝트 규칙: Core/HOG_Debug.h 사용
        Debug::Print(FString::Printf(TEXT("[LockOn] Target=%s Dist=%.0f Angle=%.1f Score=%.2f Tags=%s"),
            *GetNameSafe(BestActor),
            BestDist,
            BestAngle,
            BestScore,
            *BestTags.ToStringSimple()
        ));
    }

    return true;
}







