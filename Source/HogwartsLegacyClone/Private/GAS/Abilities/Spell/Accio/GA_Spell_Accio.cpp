// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/Spell/Accio/GA_Spell_Accio.h"

#include "HOGDebugHelper.h"
#include "Core/HOG_GameplayTags.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

UGA_Spell_Accio::UGA_Spell_Accio()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Spell_Accio::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (TryBeginPreCastFacing(Handle, ActorInfo, ActivationInfo, TriggerEventData))
	{
		return;
	}

	ExecuteAccioCast(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Spell_Accio::OnPreCastFacingFinished(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ExecuteAccioCast(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Spell_Accio::ExecuteAccioCast(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);

	if (CastVoiceSound && Character)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastVoiceSound, Character->GetActorLocation());
	}

	if (Character && Character->GetMesh() && CastMontage)
	{
		UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			const float Duration = AnimInstance->Montage_Play(CastMontage, 3.f);
			if (Duration > 0.f)
			{
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &UGA_Spell_Accio::OnMontageEnded);
				AnimInstance->Montage_SetEndDelegate(EndDelegate, CastMontage);

				// 타겟이 없으면 즉시 어빌리티 종료
				if (!FireAccio())
				{
					EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
				}
				// 성공했다면 EndAbility를 부르지 않음
				return;
			}
		}
	}

	if (!FireAccio())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

// 마법 시전 중 다시 Accio 키를 눌렀을 때의 (토글 오프) 로직
void UGA_Spell_Accio::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	// 끌어당기고 있는 대상이 있다면 유저의 판단으로 강제 종료
	if (IsValid(TargetToMove))
	{
		Debug::Print(TEXT("[Accio] Canceled by Toggle Input."), FColor::Yellow);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

bool UGA_Spell_Accio::FireAccio()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar) return false;

	FGameplayTagContainer TargetTags;
	FVector AimPoint;
	AActor* AcquiredTarget = nullptr;

	// 1. LockOn 컴포넌트를 이용해 타겟팅하거나 정면 트레이스 수행
	bool bHasTarget = TryConsumeLockedTarget(AcquiredTarget, TargetTags, AimPoint);

	if (!IsValid(AcquiredTarget))
	{
		UWorld* World = GetWorld();
		if (World)
		{
			FVector StartLoc = Avatar->GetActorLocation();
			FVector TargetLoc = AimPoint.IsNearlyZero() ? StartLoc + (Avatar->GetActorForwardVector() * GetCastRange()) : AimPoint;

			// 탐지 반경
			float SweepRadius = 50.f; 
			FCollisionShape SphereShape = FCollisionShape::MakeSphere(SweepRadius);
			FCollisionQueryParams Params(SCENE_QUERY_STAT(AccioTrace), false, Avatar);

			// ======== [디버그 드로우: 트레이스 출발선과 도착구형] ========
			DrawDebugLine(World, StartLoc, TargetLoc, FColor::Green, false, 2.0f, 0, 2.0f);
			DrawDebugSphere(World, StartLoc, SweepRadius, 16, FColor::Green, false, 2.0f);
			DrawDebugSphere(World, TargetLoc, SweepRadius, 16, FColor::Green, false, 2.0f);
			// =========================================================

			// 바닥 등에 막히는 현상 방지를 위해 MultiSweep으로 변경
			TArray<FHitResult> HitResults;
			bool bHit = World->SweepMultiByChannel(HitResults, StartLoc, TargetLoc, FQuat::Identity, ECC_Visibility, SphereShape, Params);

			if (bHit)
			{
				for (const FHitResult& Hit : HitResults)
				{
					// ======== [디버그 드로우: 스캔에 걸린 모든 지점 빨간 점] ========
					DrawDebugPoint(World, Hit.ImpactPoint, 15.f, FColor::Red, false, 2.0f);
					// =============================================================

					AActor* HitActor = Hit.GetActor();
					if (!IsValid(HitActor) || HitActor == Avatar) continue;

					UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
					bool bIsTargetNode = TargetASC && TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioTarget);

					// == Root가 아니더라도 물리가 켜진 컴포넌트가 있는지 검사 ==
					bool bIsSimulatingPhysics = false;
					TArray<UPrimitiveComponent*> PrimitiveComps;
					HitActor->GetComponents<UPrimitiveComponent>(PrimitiveComps);
					for(UPrimitiveComponent* PrimComp : PrimitiveComps)
					{
						if (PrimComp->IsSimulatingPhysics())
						{
							bIsSimulatingPhysics = true;
							break;
						}
					}

					bool bIsMovable = HitActor->IsA<ACharacter>() || bIsSimulatingPhysics;

					if (bIsTargetNode || bIsMovable)
					{
						AcquiredTarget = HitActor;
						DrawDebugBox(World, AcquiredTarget->GetActorLocation(), FVector(60.f), FColor::Cyan, false, 2.0f);
						break; 
					}
				}
			}
		}
	}

	if (!IsValid(AcquiredTarget)) 
	{
		Debug::Print(TEXT("[Accio] No Pullable Target Found."), FColor::Yellow);
		return false;
	}

	OriginalTarget = AcquiredTarget;
	TargetToMove = nullptr;
	PullDestination = nullptr;
	bIsPullingInteractable = false; // 기본값 초기화
	CurrentPullSpeed = DefaultPullSpeed;

	// 플레이어가 현재 어떤 바닥을 밟고 있는지 확인
	ACharacter* AvatarChar = Cast<ACharacter>(Avatar);
	UPrimitiveComponent* MovementBase = AvatarChar ? AvatarChar->GetCharacterMovement()->GetMovementBase() : nullptr;
	AActor* CurrentFloorActor = MovementBase ? MovementBase->GetOwner() : nullptr;

	// ASC를 가져와서 태그 검사
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AcquiredTarget);
	bool bIsHitTarget = TargetASC && TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioTarget);

	UAbilitySystemComponent* FloorASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(CurrentFloorActor);
	bool bIsStandingOnPlatform = FloorASC && FloorASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioPlatform);
	
	// =========================
	// 최소 수정:
	// "타겟팅은 됐지만 Accio로 실제 이동 가능한 대상은 아닌 경우" 차단
	// 예: Rideable 같은 다른 Interactable 태그만 있고,
	//     Accio 관련 태그/Enemy 태그는 없는 경우
	// =========================
	bool bCanBeMovedByAccioTag = false;

	if (TargetASC)
	{
		if (TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioTarget) ||
			TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioPlatform) ||
			TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Team_Enemy) ||
			TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Unit.Enemy"))))
		{
			bCanBeMovedByAccioTag = true;
		}
	}

	if (!bCanBeMovedByAccioTag)
	{
		Debug::Print(
			FString::Printf(TEXT("[Accio] Target is targetable but not movable by Accio tag: %s"), *GetNameSafe(AcquiredTarget)),
			FColor::Red
		);
		return false;
	}

	// [대상 식별] 적(Enemy)인지, 상호작용 물체(Interactable)인지 판별
	if (TargetASC)
	{
		// 태그 계층구조를 활용해 "Interactable" 하위 태그가 있는지, "Unit.Enemy" 하위 태그가 있는지 확인
		if (TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioPlatform) ||
	TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioTarget))
		{
			bIsPullingInteractable = true;
			CurrentPullSpeed = InteractablePullSpeed; // 물체/발판 등은 부드럽게
		}
		else if (TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Unit.Enemy"))) || 
				 TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Team_Enemy))
		{
			bIsPullingInteractable = false;
			CurrentPullSpeed = EnemyPullSpeed; // 적은 빠르고 강렬하게 당김
		}
	}

	// [규칙 2] 맞춘 대상이 'AccioTarget' 이고, 밟고 있는 바닥이 'AccioPlatform' 일 때
	if (bIsHitTarget)
	{
		if (bIsStandingOnPlatform)
		{
			// 플레이어가 밟고 있는 발판이 타겟을 향해 이동
			TargetToMove = CurrentFloorActor;
			PullDestination = AcquiredTarget;
			Debug::Print(TEXT("[Accio] Platform -> Target Pulling!"), FColor::Magenta);
		}
		else
		{
			Debug::Print(TEXT("[Accio] Cannot pull Accio Target directly. Get on a platform first!"), FColor::Red);
			return false;
		}
	}
	else
	{
		// [규칙 1 & Default] 발판이든 적군이든 어떤 물체든 나(플레이어)를 향해 당김
		TargetToMove = AcquiredTarget;
		PullDestination = Avatar;
		Debug::Print(FString::Printf(TEXT("[Accio] Pulling %s to Avatar"), *TargetToMove->GetName()), FColor::Cyan);
	}

	if (AccioVFX && IsValid(OriginalTarget))
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), AccioVFX, OriginalTarget->GetActorLocation());
	}

	// 물체의 성질에 따라 중력 무시 처리 세팅
	if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetToMove))
	{
		TargetCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	}
	//IsSimulatingPhysics()인 구체적 컴포넌트를 찾아서 중력 설정 ==
	else
	{
		TArray<UPrimitiveComponent*> PrimitiveComps;
		TargetToMove->GetComponents<UPrimitiveComponent>(PrimitiveComps);
		for(UPrimitiveComponent* MovePrimComp : PrimitiveComps)
		{
			if (MovePrimComp->IsSimulatingPhysics())
			{
				UAbilitySystemComponent* MoveASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetToMove);
				bool bIsPlatform = MoveASC && MoveASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioPlatform);

				if (!bIsPlatform)
				{
					MovePrimComp->SetEnableGravity(false);
				}
				break; // 보통 물리 컴포넌트는 하나만 조작하므로 찾으면 break
			}
		}
	}

	GetWorld()->GetTimerManager().SetTimer(PullTimerHandle, this, &UGA_Spell_Accio::UpdatePulling, 0.02f, true);
	return true;
}

void UGA_Spell_Accio::UpdatePulling()
{
	if (!IsValid(TargetToMove) || !IsValid(PullDestination))
	{
		GetWorld()->GetTimerManager().ClearTimer(PullTimerHandle);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 1. 목적지(DestLoc) 계산: 
	// 타겟팅 된 목적지 객체도 물리에 의해(혹은 루트 분리로) 이동했을 가능성을 대비해 메쉬 위치 우선 탐색
	FVector DestLoc = PullDestination->GetActorLocation();
	if (!PullDestination->IsA<ACharacter>())
	{
		TArray<UPrimitiveComponent*> DestPrimComps;
		PullDestination->GetComponents<UPrimitiveComponent>(DestPrimComps);
		for (UPrimitiveComponent* PrimComp : DestPrimComps)
		{
			if (PrimComp->IsA<UStaticMeshComponent>() || PrimComp->IsSimulatingPhysics())
			{
				DestLoc = PrimComp->GetComponentLocation();
				break;
			}
		}
	}

	// 2. 당겨지는 객체(MoveLoc) 위치 계산:
	// 물리를 시뮬레이션 중인 구체적인 자식 컴포넌트를 찾아 실제 이동한 컴포넌트 위치를 가져옴
	FVector MoveLoc = TargetToMove->GetActorLocation();
	UPrimitiveComponent* ActualMoveComp = nullptr;

	if (!TargetToMove->IsA<ACharacter>())
	{
		TArray<UPrimitiveComponent*> MovePrimComps;
		TargetToMove->GetComponents<UPrimitiveComponent>(MovePrimComps);
		for (UPrimitiveComponent* MovePrimComp : MovePrimComps)
		{
			if (MovePrimComp->IsSimulatingPhysics())
			{
				MoveLoc = MovePrimComp->GetComponentLocation(); // 갱신된 현재 위치
				ActualMoveComp = MovePrimComp;
				break;
			}
		}
	}

	// 거리 계산 및 정지 처리
	float Distance = FVector::Dist2D(DestLoc, MoveLoc);

	if (Distance <= StopDistance)
	{
		GetWorld()->GetTimerManager().ClearTimer(PullTimerHandle);

		if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetToMove))
		{
			TargetCharacter->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		}
		else if (ActualMoveComp)
		{
			ActualMoveComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
		}
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 방향 연산
	FVector Direction = (DestLoc - MoveLoc);
	Direction.Z = 0.f;
	FVector PullDirection = Direction.GetSafeNormal();

	// 속도 적용
	if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetToMove))
	{
		TargetCharacter->GetCharacterMovement()->Velocity = PullDirection * CurrentPullSpeed;
	}
	else if (ActualMoveComp)
	{
		ActualMoveComp->SetPhysicsLinearVelocity(PullDirection * CurrentPullSpeed);
	}
}

void UGA_Spell_Accio::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted || !IsValid(TargetToMove))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
	}
}

void UGA_Spell_Accio::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(PullTimerHandle);

	if (IsValid(TargetToMove))
	{
		if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetToMove))
		{
			TargetCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		}
		// == 종료 시 물리/중력 복구 대상 ==
		else
		{
			TArray<UPrimitiveComponent*> PrimitiveComps;
			TargetToMove->GetComponents<UPrimitiveComponent>(PrimitiveComps);
			for(UPrimitiveComponent* MovePrimComp : PrimitiveComps)
			{
				if (MovePrimComp->IsSimulatingPhysics())
				{
					UAbilitySystemComponent* MoveASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetToMove);
					bool bIsPlatform = MoveASC && MoveASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioPlatform);
					
					if (!bIsPlatform)
					{
						MovePrimComp->SetEnableGravity(true);
					}
					
					MovePrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
					break;
				}
			}
		}

		TargetToMove = nullptr;
		PullDestination = nullptr;
		OriginalTarget = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}