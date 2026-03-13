// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/Spell/Accio/GA_Spell_Accio.h"
#include "HOGDebugHelper.h"
#include "Core/HOG_GameplayTags.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/World.h"
#include "TimerManager.h"

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

	OriginalTarget = AcquiredTarget;
	TargetToMove = nullptr;
	PullDestination = nullptr;

	// 플레이어가 현재 어떤 바닥을 밟고 있는지 확인
	ACharacter* AvatarChar = Cast<ACharacter>(Avatar);
	UPrimitiveComponent* MovementBase = AvatarChar ? AvatarChar->GetCharacterMovement()->GetMovementBase() : nullptr;
	AActor* CurrentFloorActor = MovementBase ? MovementBase->GetOwner() : nullptr;

	// ASC를 가져와서 태그 검사
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AcquiredTarget);
	bool bIsHitTarget = TargetASC && TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioTarget);

	UAbilitySystemComponent* FloorASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(CurrentFloorActor);
	bool bIsStandingOnPlatform = FloorASC && FloorASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioPlatform);

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
	else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(TargetToMove->GetRootComponent()))
	{
		// 플랫폼 여부 검사
		UAbilitySystemComponent* MoveASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetToMove);
		bool bIsPlatform = MoveASC && MoveASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioPlatform);

		if (!bIsPlatform && PrimComp->IsSimulatingPhysics())
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

	// 높낮이 방지 (Z축 제외)
	FVector Direction = (DestLoc - MoveLoc);
	Direction.Z = 0.f; 
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
			// ASC 기반 태그 검사로 변경됨
			UAbilitySystemComponent* MoveASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetToMove);
			bool bIsPlatform = MoveASC && MoveASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioPlatform);
			
			if (!bIsPlatform && PrimComp->IsSimulatingPhysics())
			{
				PrimComp->SetEnableGravity(true);
			}
			
			if (PrimComp->IsSimulatingPhysics())
			{
				PrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
			}
		}

		TargetToMove = nullptr;
		PullDestination = nullptr;
		OriginalTarget = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}