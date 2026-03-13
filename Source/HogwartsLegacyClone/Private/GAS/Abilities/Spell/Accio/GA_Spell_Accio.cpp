// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/Spell/Accio/GA_Spell_Accio.h"
#include "HOGDebugHelper.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Interactable/InteractableAccioPlatform.h"
#include "Interactable/InteractableAccioTarget.h"

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

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());

	if (CastVoiceSound && Character)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastVoiceSound, Character->GetActorLocation());
	}

	if (Character && Character->GetMesh() && CastMontage)
	{
		UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			const float Duration = AnimInstance->Montage_Play(CastMontage, 1.f);
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
				// 성공했다면 EndAbility를 부르지 않음 -> 어빌리티(마법)가 계속 켜져있음!
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
	bool bHasTarget = AcquireTargetFromLockOn(AcquiredTarget, TargetTags, AimPoint);

	if (!IsValid(AcquiredTarget))
	{
		UWorld* World = GetWorld();
		if (World)
		{
			FVector StartLoc = Avatar->GetActorLocation();
			FVector TargetLoc = AimPoint.IsNearlyZero() ? StartLoc + (Avatar->GetActorForwardVector() * GetCastRange()) : AimPoint;

			FCollisionShape SphereShape = FCollisionShape::MakeSphere(100.f);
			FCollisionQueryParams Params(SCENE_QUERY_STAT(AccioTrace), false, Avatar);

			FHitResult HitResult;
			bool bHit = World->SweepSingleByChannel(HitResult, StartLoc, TargetLoc, FQuat::Identity, ECC_Visibility, SphereShape, Params);

			if (bHit && IsValid(HitResult.GetActor()))
			{
				AcquiredTarget = HitResult.GetActor();
			}
		}
	}

	if (!IsValid(AcquiredTarget)) return false;

	// 목적지와 이동 대상을 분석
	OriginalTarget = AcquiredTarget;
	TargetToMove = nullptr;
	PullDestination = nullptr;

	// 플레이어가 현재 어떤 바닥을 밟고 있는지 확인
	ACharacter* AvatarChar = Cast<ACharacter>(Avatar);
	UPrimitiveComponent* MovementBase = AvatarChar ? AvatarChar->GetCharacterMovement()->GetMovementBase() : nullptr;
	AActor* CurrentFloorActor = MovementBase ? MovementBase->GetOwner() : nullptr;

	// [규칙 2] 타겟(Target)에 Accio 사용 + 플레이어가 발판(Platform) 위일 때
	AInteractableAccioTarget* HitAccioTarget = Cast<AInteractableAccioTarget>(AcquiredTarget);
	AInteractableAccioPlatform* StandingPlatform = Cast<AInteractableAccioPlatform>(CurrentFloorActor);

	if (HitAccioTarget)
	{
		if (StandingPlatform)
		{
			// 플레이어가 밟고 있는 발판이 타겟을 향해 이동
			TargetToMove = StandingPlatform;
			PullDestination = HitAccioTarget;
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
		// [규칙 1 & Default] 발판이든 적군이든 타겟을 나(플레이어)를 향해 당김
		TargetToMove = AcquiredTarget;
		PullDestination = Avatar;
		Debug::Print(FString::Printf(TEXT("[Accio] Pulling %s to Avatar"), *TargetToMove->GetName()), FColor::Cyan);
	}

	// ----------------------------------------------------
	
	if (AccioVFX) UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), AccioVFX, OriginalTarget->GetActorLocation());

	// 플랫폼이 아니며 물리/중력 무력화가 필요한 경우만 (플랫폼은 Z축 이미 락걸려 있음)
	if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetToMove))
	{
		TargetCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	}
	else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(TargetToMove->GetRootComponent()))
	{
		if (!TargetToMove->IsA(AInteractableAccioPlatform::StaticClass()) && PrimComp->IsSimulatingPhysics())
		{
			PrimComp->SetEnableGravity(false);
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

	FVector DestLoc = PullDestination->GetActorLocation();
	FVector MoveLoc = TargetToMove->GetActorLocation();

	float Distance = FVector::Dist2D(DestLoc, MoveLoc);

	if (Distance <= StopDistance)
	{
		GetWorld()->GetTimerManager().ClearTimer(PullTimerHandle);

		if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetToMove))
		{
			TargetCharacter->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		}
		else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(TargetToMove->GetRootComponent()))
		{
			if (PrimComp->IsSimulatingPhysics())
			{
				PrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
			}
		}
		return; 
	}

	// 높낮이 방지 (로테이션/높이 불변)
	FVector Direction = (DestLoc - MoveLoc);
	Direction.Z = 0.f; // Z축으로 당기는 힘 제거 (서로 동일선상에서 당김)
	FVector PullDirection = Direction.GetSafeNormal();

	// 속도 적용
	if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetToMove))
	{
		TargetCharacter->GetCharacterMovement()->Velocity = PullDirection * PullSpeed;
	}
	else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(TargetToMove->GetRootComponent()))
	{
		if (PrimComp->IsSimulatingPhysics())
		{
			PrimComp->SetPhysicsLinearVelocity(PullDirection * PullSpeed);
		}
		else
		{
			TargetToMove->SetActorLocation(MoveLoc + (PullDirection * PullSpeed * 0.02f));
		}
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
		else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(TargetToMove->GetRootComponent()))
		{
			// 플랫폼이 아닌 일반물체인 경우에만 중력 다시 복구
			if (!TargetToMove->IsA(AInteractableAccioPlatform::StaticClass()) && PrimComp->IsSimulatingPhysics())
			{
				PrimComp->SetEnableGravity(true);
			}
            PrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
		}

		TargetToMove = nullptr;
		PullDestination = nullptr;
		OriginalTarget = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}