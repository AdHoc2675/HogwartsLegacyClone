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

// 👉 [추가] 마법 시전 중 다시 Accio 키를 눌렀을 때의 (토글 오프) 로직
void UGA_Spell_Accio::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	// 끌어당기고 있는 대상이 있다면 유저의 판단으로 강제 종료시킵니다.
	if (IsValid(PulledTarget))
	{
		Debug::Print(TEXT("[Accio] Canceled by Toggle Input. Dropping Target."), FColor::Yellow);
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

	// 2. 대상을 찾았으면 끌어오기 시작
	if (IsValid(AcquiredTarget))
	{
		PulledTarget = AcquiredTarget;
		Debug::Print(FString::Printf(TEXT("[Accio] Target Acquired: %s"), *PulledTarget->GetName()), FColor::Cyan);

		if (AccioVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), AccioVFX, PulledTarget->GetActorLocation());
		}

		// 마찰력과 중력 무력화 (공중에 살짝 띄운 상태로 끌려오게 연출)
		if (ACharacter* TargetCharacter = Cast<ACharacter>(PulledTarget))
		{
			if (UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement())
			{
				MoveComp->SetMovementMode(MOVE_Flying); // 하늘을 나는 모드로 전환
			}
		}
		else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(PulledTarget->GetRootComponent()))
		{
			if (PrimComp->IsSimulatingPhysics())
			{
				PrimComp->SetEnableGravity(false);
			}
		}

		// 3. 타이머 등록 (0.02초, 50fps 속도로 지속해서 목표를 당긴다)
		GetWorld()->GetTimerManager().SetTimer(PullTimerHandle, this, &UGA_Spell_Accio::UpdatePulling, 0.02f, true);
		return true; // 성공
	}
	else
	{
		Debug::Print(TEXT("[Accio] No Target."), FColor::Yellow);
		return false; // 실패
	}
}

void UGA_Spell_Accio::UpdatePulling()
{
	// 👉 대상이 죽었거나 파괴되면 자동으로 어빌리티 종료
	if (!IsValid(PulledTarget))
	{
		GetWorld()->GetTimerManager().ClearTimer(PullTimerHandle);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar) return;

	FVector AvatarLoc = Avatar->GetActorLocation();
	FVector TargetLoc = PulledTarget->GetActorLocation();

	// 거리를 계산 (내 앞에 도달하면 정지)
	float Distance = FVector::Dist2D(AvatarLoc, TargetLoc);

	if (Distance <= StopDistance)
	{
		// 유저 앞까지 왔을 땐 당기는 힘만 0으로 만들고 어빌리티는 유지(공중에 띄움)
		GetWorld()->GetTimerManager().ClearTimer(PullTimerHandle);

		if (ACharacter* TargetCharacter = Cast<ACharacter>(PulledTarget))
		{
			if (UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement())
			{
				MoveComp->Velocity = FVector::ZeroVector;
			}
		}
		else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(PulledTarget->GetRootComponent()))
		{
			if (PrimComp->IsSimulatingPhysics())
			{
				PrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
			}
		}

		return; // 아직 중력 회복(EndAbility)은 시키지 않음 (콤보나 공격 대기 상태로 공중에 머물도록)
	}

	// 플레이어 방향 향하는 벡터 구하기
	// 높이(Z)는 대상 원본 높이나 내 가슴(chest) 높이 정도로 보정하면 예쁘게 당겨짐
	FVector Direction = (AvatarLoc - TargetLoc);
	Direction.Z = 0.f; // Z축 보정을 위해 수평 당김만 할지 설정. 약간 띄우려면 Z 조작 추가 가능.
	FVector PullDirection = Direction.GetSafeNormal();

	// 매 프레임 플레이어 방향으로 속도값 덮어씌움
	if (ACharacter* TargetCharacter = Cast<ACharacter>(PulledTarget))
	{
		if (UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement())
		{
			MoveComp->Velocity = PullDirection * PullSpeed;
		}
	}
	else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(PulledTarget->GetRootComponent()))
	{
		if (PrimComp->IsSimulatingPhysics())
		{
			PrimComp->SetPhysicsLinearVelocity(PullDirection * PullSpeed);
		}
		else
		{
			// 물리가 꺼진 오브젝트(예: 퍼즐 블록)의 경우 강제 위치 이동
			PulledTarget->SetActorLocation(TargetLoc + (PullDirection * PullSpeed * 0.02f));
		}
	}
}

void UGA_Spell_Accio::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// 👉 몽타주가 끝났어도 마법 대상이 있다면 종료하지 않음. 
	// 도중에 맞아서 취소되거나 타겟이 사라졌을 때만 종료
	if (bInterrupted || !IsValid(PulledTarget))
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
	// 4. 어빌리티 종료 시점 -> 다시 물리 및 중력값 정상화
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PullTimerHandle);
	}

	if (IsValid(PulledTarget))
	{
		if (ACharacter* TargetCharacter = Cast<ACharacter>(PulledTarget))
		{
			if (UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement())
			{
				MoveComp->SetMovementMode(MOVE_Falling); // 다시 바닥으로 떨어지게 함
			}
		}
		else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(PulledTarget->GetRootComponent()))
		{
			if (PrimComp->IsSimulatingPhysics())
			{
				PrimComp->SetEnableGravity(true);
			}
		}

		PulledTarget = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}