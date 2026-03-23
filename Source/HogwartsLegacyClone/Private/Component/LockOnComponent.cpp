#include "Component/LockOnComponent.h"

#include "HOGDebugHelper.h"
// 프로젝트 표준이 Core/HOG_Debug.h 라면 위 include를 그 헤더로 교체

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

#include "Components/PrimitiveComponent.h"

#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"

#include "HOGDebugHelper.h"
#include "Core/HOG_GameplayTags.h"

ULockOnComponent::ULockOnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void ULockOnComponent::BeginPlay()
{
	Super::BeginPlay();

	TimeSinceLastRefresh = 0.f;

	SetComponentTickEnabled(true);
	PrimaryComponentTick.SetTickFunctionEnable(true);
}

void ULockOnComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearCurrentTarget();
	Super::EndPlay(EndPlayReason);
}

void ULockOnComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ShouldRunRealtimeTargeting())
	{
		return;
	}

	TimeSinceLastRefresh += DeltaTime;

	if (TimeSinceLastRefresh < RealtimeUpdateInterval)
	{
		return;
	}

	TimeSinceLastRefresh = 0.f;
	RefreshCurrentTarget();
}

bool ULockOnComponent::ShouldRunRealtimeTargeting() const
{
	if (!bEnableRealtimeTargeting)
	{
		return false;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return false;
	}

	if (bRealtimeOnlyWhenLocallyControlled && !OwnerPawn->IsLocallyControlled())
	{
		return false;
	}

	return true;
}

bool ULockOnComponent::GetCameraView(FVector& OutCamLoc, FVector& OutCamForward) const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return false;
	}

	const APlayerController* PC = Cast<APlayerController>(OwnerPawn->GetController());
	if (!PC)
	{
		return false;
	}

	const APlayerCameraManager* CamMgr = PC->PlayerCameraManager;
	if (!CamMgr)
	{
		return false;
	}

	OutCamLoc = CamMgr->GetCameraLocation();
	OutCamForward = CamMgr->GetActorForwardVector();
	OutCamForward = OutCamForward.GetSafeNormal();
	return true;
}

bool ULockOnComponent::GetCenterAimPoint(FVector& OutAimPoint) const
{
	FVector CamLoc, CamForward;
	if (!GetCameraView(CamLoc, CamForward))
	{
		return false;
	}

	OutAimPoint = CamLoc + (CamForward * MaxRange);
	return true;
}

float ULockOnComponent::ComputeScore(float AngleDeg, float Dist) const
{
	const float Angle01 = 1.f - FMath::Clamp(AngleDeg / FMath::Max(0.01f, MaxAngleDegrees), 0.f, 1.f);
	const float Dist01 = 1.f - FMath::Clamp(Dist / FMath::Max(1.f, MaxRange), 0.f, 1.f);
	return (Angle01 * AngleWeight) + (Dist01 * DistanceWeight);
}

bool ULockOnComponent::HasLineOfSight(const FVector& CamLoc, AActor* Candidate) const
{
	if (!bRequireLineOfSight)
	{
		return true;
	}

	if (!Candidate)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

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

	if (!bHit)
	{
		return true;
	}

	return (Hit.GetActor() == Candidate);
}

bool ULockOnComponent::HasValidCurrentTarget() const
{
	return bHasValidTarget && (CurrentTargetActor.Get() != nullptr);
}

bool ULockOnComponent::TryGetLockedTargetResult(FLockOnTargetResult& OutResult) const
{
	OutResult = FLockOnTargetResult();

	if (!HasValidCurrentTarget())
	{
		return false;
	}

	if (!IsValid(CurrentTargetActor.Get()))
	{
		return false;
	}

	OutResult = CurrentTargetResult;
	return IsValid(OutResult.TargetActor);
}

void ULockOnComponent::ForceRefreshTarget()
{
	RefreshCurrentTarget();
}

void ULockOnComponent::ClearCurrentTarget()
{
	if (CurrentTargetActor.Get() != nullptr)
	{
		ApplyTargetOutline(CurrentTargetActor.Get(), CurrentTargetResult.TargetTags, false);
		OnLockOnReleased.Broadcast(CurrentTargetResult);
	}

	CurrentTargetActor = nullptr;
	CurrentTargetResult = FLockOnTargetResult();
	bHasValidTarget = false;
}

void ULockOnComponent::SetCurrentTarget(AActor* NewTarget, const FLockOnTargetResult& NewResult)
{
	const AActor* PrevTarget = CurrentTargetActor.Get();

	if (PrevTarget == NewTarget)
	{
		CurrentTargetResult = NewResult;
		bHasValidTarget = (NewTarget != nullptr);


		return;
	}


	if (CurrentTargetActor.Get() != nullptr)
	{
		ApplyTargetOutline(CurrentTargetActor.Get(), CurrentTargetResult.TargetTags, false);
	}

	CurrentTargetActor = NewTarget;
	CurrentTargetResult = NewResult;
	bHasValidTarget = (NewTarget != nullptr);

	if (CurrentTargetActor.Get() != nullptr)
	{
		ApplyTargetOutline(CurrentTargetActor.Get(), CurrentTargetResult.TargetTags, true);
		OnLockOnTarget.Broadcast(CurrentTargetResult);
	}
}

void ULockOnComponent::RefreshCurrentTarget()
{
	FLockOnTargetResult NewResult;
	const bool bFound = FindBestTargetInternal(AllowedTargetTags, NewResult);

	if (!bFound || !IsValid(NewResult.TargetActor))
	{
		ClearCurrentTarget();
		return;
	}


	SetCurrentTarget(NewResult.TargetActor, NewResult);
}

bool ULockOnComponent::FindBestTarget(
	const FGameplayTagContainer& RequiredTargetTags,
	FLockOnTargetResult& OutResult
) const
{
	return FindBestTargetInternal(RequiredTargetTags, OutResult);
}

bool ULockOnComponent::FindBestTargetInternal(
	const FGameplayTagContainer& RequiredTargetTags,
	FLockOnTargetResult& OutResult
) const
{
	OutResult = FLockOnTargetResult();

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FVector CamLoc, CamForward;
	if (!GetCameraView(CamLoc, CamForward))
	{
		return false;
	}

	TArray<AActor*> Candidates;
	GatherCandidateActors(Candidates);

	if (Candidates.Num() == 0)
	{
		GetCenterAimPoint(OutResult.AimPoint);
		return false;
	}

	float BestScore = -1.f;
	FLockOnTargetResult BestResult;

	for (AActor* Candidate : Candidates)
	{
		FLockOnTargetResult CandidateResult;
		if (!EvaluateCandidate(Candidate, CamLoc, CamForward, RequiredTargetTags, CandidateResult))
		{
			continue;
		}

		if (CandidateResult.Score > BestScore)
		{
			BestScore = CandidateResult.Score;
			BestResult = CandidateResult;
		}
	}

	if (!IsValid(BestResult.TargetActor))
	{
		GetCenterAimPoint(OutResult.AimPoint);
		return false;
	}

	OutResult = BestResult;
	return true;
}

void ULockOnComponent::GatherCandidateActors(TArray<AActor*>& OutCandidates) const
{
	OutCandidates.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector CamLoc, CamForward;
	if (!GetCameraView(CamLoc, CamForward))
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;

	FCollisionObjectQueryParams ObjParams;
	if (bIncludePawnTargets)
	{
		ObjParams.AddObjectTypesToQuery(ECC_Pawn);
	}
	if (bIncludeWorldDynamicTargets)
	{
		ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	}

	// 물리 오브젝트(PhysicsActor 프로필 등)도 락온 후보에 포함
	ObjParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	if (ObjParams.IsValid() == false)
	{
		return;
	}

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

	if (!bAnyOverlap)
	{
		return;
	}

	TSet<AActor*> UniqueActors;
	for (const FOverlapResult& O : Overlaps)
	{
		AActor* Candidate = O.GetActor();
		if (!Candidate) continue;
		if (Candidate == GetOwner()) continue;

		UniqueActors.Add(Candidate);
	}

	for (AActor* UniqueActor : UniqueActors)
	{
		OutCandidates.Add(UniqueActor);
	}
}

bool ULockOnComponent::EvaluateCandidate(
	AActor* Candidate,
	const FVector& CamLoc,
	const FVector& CamForward,
	const FGameplayTagContainer& RequiredTargetTags,
	FLockOnTargetResult& OutCandidateResult
) const
{
	OutCandidateResult = FLockOnTargetResult();

	if (!Candidate)
	{
		return false;
	}

	const FVector ToCandidate = Candidate->GetActorLocation() - CamLoc;
	const float Dist = ToCandidate.Size();
	if (Dist <= KINDA_SMALL_NUMBER || Dist > MaxRange)
	{
		return false;
	}

	const FVector Dir = ToCandidate / Dist;
	const float Dot = FVector::DotProduct(CamForward, Dir);
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)));
	if (AngleDeg > MaxAngleDegrees)
	{
		return false;
	}

	if (!HasLineOfSight(CamLoc, Candidate))
	{
		return false;
	}

	FGameplayTagContainer CandidateTags;
	if (!IsTargetCandidate(Candidate, RequiredTargetTags, CandidateTags))
	{
		return false;
	}

	OutCandidateResult.TargetActor = Candidate;
	OutCandidateResult.TargetTags = CandidateTags;
	OutCandidateResult.Distance = Dist;
	OutCandidateResult.AngleDegrees = AngleDeg;
	OutCandidateResult.Score = ComputeScore(AngleDeg, Dist);
	OutCandidateResult.AimPoint = Candidate->GetActorLocation();

	return true;
}

bool ULockOnComponent::IsTargetCandidate(
	AActor* Candidate,
	const FGameplayTagContainer& RequiredTargetTags,
	FGameplayTagContainer& OutCandidateTags
) const
{
	OutCandidateTags.Reset();

	if (!Candidate)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Candidate);
	if (!ASC)
	{
		return false;
	}

	ASC->GetOwnedGameplayTags(OutCandidateTags);

	if (RequiredTargetTags.IsEmpty())
	{
		return true;
	}

	return OutCandidateTags.HasAny(RequiredTargetTags);
}

void ULockOnComponent::ResolveTargetPrimitiveComponents(
	AActor* TargetActor,
	TArray<UPrimitiveComponent*>& OutComponents
) const
{
	OutComponents.Reset();

	if (!TargetActor)
	{
		return;
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	TargetActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* PrimitiveComp : PrimitiveComponents)
	{
		if (!PrimitiveComp)
		{
			continue;
		}

		if (!PrimitiveComp->IsRegistered())
		{
			continue;
		}

		if (!PrimitiveComp->IsVisible())
		{
			continue;
		}

		OutComponents.Add(PrimitiveComp);
	}
}

int32 ULockOnComponent::GetOutlineStencilValueForTarget(const FGameplayTagContainer& TargetTags) const
{
	static const FGameplayTag TeamObjectTag = FGameplayTag::RequestGameplayTag(TEXT("Team.Object"), false);
	static const FGameplayTag TeamEnemyTag = FGameplayTag::RequestGameplayTag(TEXT("Team.Enemy"), false);

	if (TeamObjectTag.IsValid() && TargetTags.HasTag(TeamObjectTag))
	{
		return ObjectOutlineStencilValue;
	}

	if (TeamEnemyTag.IsValid() && TargetTags.HasTag(TeamEnemyTag))
	{
		return EnemyOutlineStencilValue;
	}

	return EnemyOutlineStencilValue;
}

void ULockOnComponent::ApplyTargetOutline(
	AActor* TargetActor,
	const FGameplayTagContainer& TargetTags,
	bool bEnable
) const
{
	if (!bUseTargetOutline)
	{
		return;
	}

	TArray<UPrimitiveComponent*> TargetComponents;
	ResolveTargetPrimitiveComponents(TargetActor, TargetComponents);


	if (TargetComponents.Num() == 0)
	{
		return;
	}

	const int32 StencilValue = GetOutlineStencilValueForTarget(TargetTags);

	for (UPrimitiveComponent* PrimitiveComp : TargetComponents)
	{
		if (!PrimitiveComp)
		{
			continue;
		}

		if (bEnable)
		{
			PrimitiveComp->SetRenderCustomDepth(true);
			PrimitiveComp->SetCustomDepthStencilValue(StencilValue);
		}
		else
		{
			PrimitiveComp->SetRenderCustomDepth(false);
		}
	}
}