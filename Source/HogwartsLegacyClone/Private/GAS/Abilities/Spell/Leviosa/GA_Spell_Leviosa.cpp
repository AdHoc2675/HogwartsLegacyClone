// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/Spell/Leviosa/GA_Spell_Leviosa.h"
#include "HOGDebugHelper.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"

UGA_Spell_Leviosa::UGA_Spell_Leviosa()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Spell_Leviosa::ActivateAbility(
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

	// 1. 주문 시전 사운드 재생
	if (CastVoiceSound && Character)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastVoiceSound, Character->GetActorLocation());
	}

	// 2. 타겟 획득 (LockOn 우선)
	FGameplayTagContainer TargetTags;
	FVector AimPoint;
	AActor* AcquiredTarget = nullptr;

	bool bHasTarget = AcquireTargetFromLockOn(AcquiredTarget, TargetTags, AimPoint);

	// 락온된 적(Pawn)이 없다면, 시야 정방향(AimPoint)으로 트레이스를 날려 사물 탐지
	if (!IsValid(AcquiredTarget) && Character)
	{
		UWorld* World = Character->GetWorld();
		if (World)
		{
			FVector StartLoc = Character->GetActorLocation();
			// AimPoint가 0이면 정면, 아니면 AimPoint 방향으로 뻗어나감
			FVector TargetLoc = AimPoint.IsNearlyZero() ? StartLoc + (Character->GetActorForwardVector() * GetCastRange()) : AimPoint;

			FCollisionShape SphereShape = FCollisionShape::MakeSphere(100.f); // 약간 널널한 구체 판정
			FCollisionQueryParams Params(SCENE_QUERY_STAT(LeviosaObjectTrace), false, Character);

			FHitResult HitResult;
			// ECC_Visibility 채널 검사로 물리 사물(상자, 돌 등)을 감지
			bool bHit = World->SweepSingleByChannel(HitResult, StartLoc, TargetLoc, FQuat::Identity, ECC_Visibility, SphereShape, Params);

			if (bHit && IsValid(HitResult.GetActor()))
			{
				AcquiredTarget = HitResult.GetActor();
			}
		}
	}

	// 최종 타겟 저장
	if (IsValid(AcquiredTarget))
	{
		LevitatedTarget = AcquiredTarget;
		Debug::Print(FString::Printf(TEXT("[Leviosa] Target Acquired: %s"), *LevitatedTarget->GetName()), FColor::Cyan);
	}
	else
	{
		Debug::Print(TEXT("[Leviosa] No valid Target. (Casting empty)"), FColor::Yellow);
	}

	// 3. 캐스팅 애니메이션 실행 및 로직 발동
	if (Character && Character->GetMesh() && CastMontage)
	{
		UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			const float Duration = AnimInstance->Montage_Play(CastMontage, 1.f);
			if (Duration > 0.f)
			{
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &UGA_Spell_Leviosa::OnMontageEnded);
				AnimInstance->Montage_SetEndDelegate(EndDelegate, CastMontage);

				StartLevitation();
				return;
			}
		}
	}

	StartLevitation();
}

void UGA_Spell_Leviosa::StartLevitation()
{
	if (!IsValid(LevitatedTarget))
	{
		Debug::Print(TEXT("[Leviosa] No valid target to levitate."), FColor::Yellow);
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	// 1. 시각 효과 & 사운드 재생
	FVector TargetLoc = LevitatedTarget->GetActorLocation();
	if (LevitateVFX)
	{
		// 타겟의 살짝 아래쪽에서 솟아오르는 이펙트를 연출
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, LevitateVFX, TargetLoc - FVector(0, 0, 50.f));
	}
	if (LevitateSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, LevitateSound, TargetLoc);
	}

	// 2. 상태이상(Gameplay Effect) 적용
	if (LevitationEffectClass)
	{
		UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
		UAbilitySystemComponent* TargetASC = LevitatedTarget->FindComponentByClass<UAbilitySystemComponent>();

		if (OwnerASC && TargetASC)
		{
			FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(LevitationEffectClass, GetAbilityLevel());
			if (SpecHandle.IsValid())
			{
				ActiveLevitationHandle = OwnerASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			}
		}
	}

	// 3. 물리 엔진을 이용해 강제로 위로 띄우기 (간단한 물리적 구현)
	if (ACharacter* TargetCharacter = Cast<ACharacter>(LevitatedTarget))
	{
		if (UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement())
		{
			MoveComp->GravityScale = 0.0f;               // 중력 무시
			MoveComp->SetMovementMode(MOVE_Flying);      // 낙하 방지
			MoveComp->Velocity = FVector(0.f, 0.f, 250.f); // 위로 살짝 띄우기
		}
	}
	else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(LevitatedTarget->GetRootComponent()))
	{
		if (PrimComp->IsSimulatingPhysics())
		{
			PrimComp->SetEnableGravity(false);
			PrimComp->SetPhysicsLinearVelocity(FVector(0.f, 0.f, 250.f));
		}
	}

	Debug::Print(FString::Printf(TEXT("[Leviosa] Levitated %s!"), *LevitatedTarget->GetName()), FColor::Cyan);
}

void UGA_Spell_Leviosa::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
}

void UGA_Spell_Leviosa::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (IsValid(LevitatedTarget))
	{
		// 1. 상태이상(GE) 제거
		if (ActiveLevitationHandle.IsValid())
		{
			if (UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo())
			{
				OwnerASC->RemoveActiveGameplayEffect(ActiveLevitationHandle);
			}
			ActiveLevitationHandle.Invalidate();
		}

		// 2. 물리/중력 복구
		if (ACharacter* TargetCharacter = Cast<ACharacter>(LevitatedTarget))
		{
			if (UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement())
			{
				MoveComp->GravityScale = 1.0f;
				MoveComp->SetMovementMode(MOVE_Falling);
			}
		}
		else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(LevitatedTarget->GetRootComponent()))
		{
			if (PrimComp->IsSimulatingPhysics())
			{
				PrimComp->SetEnableGravity(true);
			}
		}

		Debug::Print(FString::Printf(TEXT("[Leviosa] Dropped %s"), *LevitatedTarget->GetName()), FColor::Green);
		LevitatedTarget = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}