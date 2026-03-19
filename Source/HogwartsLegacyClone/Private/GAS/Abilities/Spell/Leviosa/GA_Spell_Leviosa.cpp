// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/Spell/Leviosa/GA_Spell_Leviosa.h"
#include "HOGDebugHelper.h"
#include "Core/HOG_GameplayTags.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "DrawDebugHelpers.h"

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

	bool bHasTarget = TryConsumeLockedTarget(AcquiredTarget, TargetTags, AimPoint);

	// 락온된 적(Pawn)이 없다면, 시야 정방향(AimPoint)으로 트레이스를 날려 사물 탐지
	if (!IsValid(AcquiredTarget) && Character)
	{
		UWorld* World = Character->GetWorld();
		if (World)
		{
			FVector StartLoc = Character->GetActorLocation();
			// AimPoint가 0이면 정면, 아니면 AimPoint 방향으로 뻗어나감
			FVector TargetLoc = AimPoint.IsNearlyZero() ? StartLoc + (Character->GetActorForwardVector() * GetCastRange()) : AimPoint;

			float SweepRadius = 100.f; // 탐지 반경 (필요시 조절)
			FCollisionShape SphereShape = FCollisionShape::MakeSphere(SweepRadius); 
			FCollisionQueryParams Params(SCENE_QUERY_STAT(LeviosaObjectTrace), false, Character);

			// ======== [디버그 드로우: 트레이스 출발선과 도착구형] ========
			DrawDebugLine(World, StartLoc, TargetLoc, FColor::Green, false, 2.0f, 0, 2.0f);
			DrawDebugSphere(World, StartLoc, SweepRadius, 16, FColor::Green, false, 2.0f);
			DrawDebugSphere(World, TargetLoc, SweepRadius, 16, FColor::Green, false, 2.0f);
			// =========================================================

			// 바닥 등 고정된 물체에 막히는 것을 방지하기 위해 MultiSweep 사용
			TArray<FHitResult> HitResults;
			bool bHit = World->SweepMultiByChannel(HitResults, StartLoc, TargetLoc, FQuat::Identity, ECC_Visibility, SphereShape, Params);

			if (bHit)
			{
				for (const FHitResult& Hit : HitResults)
				{
					// ======== [디버그 드로우: 스캔에 걸린 모든 충돌 지점(빨간 점)] ========
					DrawDebugPoint(World, Hit.ImpactPoint, 15.f, FColor::Red, false, 2.0f);
					// =================================================================

					AActor* HitActor = Hit.GetActor();
					if (!IsValid(HitActor) || HitActor == Character) continue;

					// 검사: 띄울 수 있는 대상인가? (살아있는 캐릭터 혹은 물리 시뮬레이션 동작 중인 개체)
					bool bIsSimulatingPhysics = false;
					TArray<UPrimitiveComponent*> PrimitiveComps;
					HitActor->GetComponents<UPrimitiveComponent>(PrimitiveComps);
					for (UPrimitiveComponent* PrimComp : PrimitiveComps)
					{
						if (PrimComp->IsSimulatingPhysics())
						{
							bIsSimulatingPhysics = true;
							break;
						}
					}

					bool bIsMovable = HitActor->IsA<ACharacter>() || bIsSimulatingPhysics;

					// 바닥 같은 StaticMesh는 패스됨
					if (bIsMovable)
					{
						AcquiredTarget = HitActor;
						
						// ======== [디버그 드로우: 최종 채택된 타겟에 박스 표시] ========
						DrawDebugBox(World, AcquiredTarget->GetActorLocation(), FVector(60.f), FColor::Cyan, false, 2.0f);
						// ===========================================================

						break; // 유효한 첫 대상을 찾았으므로 탈출
					}
				}
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
			const float Duration = AnimInstance->Montage_Play(CastMontage, 3.f);
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

	// 몽타주 재생에 실패했거나 없는데 타겟도 없다면 바로 종료
	if (!StartLevitation())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
	}
}

bool UGA_Spell_Leviosa::StartLevitation()
{
	if (!IsValid(LevitatedTarget))
	{
		Debug::Print(TEXT("[Leviosa] No valid target to levitate."), FColor::Yellow);
		return false;
	}

	UWorld* World = GetWorld();
	if (!World) return false;

	// 1. 시각 효과 & 사운드 재생
	FVector TargetLoc = LevitatedTarget->GetActorLocation();

	// 이펙트 출력을 위해 타겟의 실제 최하단 메쉬 위치를 찾아 좀 더 자연스럽게 출력 시도
	TArray<UPrimitiveComponent*> RenderComps;
	LevitatedTarget->GetComponents<UPrimitiveComponent>(RenderComps);
	for (auto Comp : RenderComps)
	{
		if (Comp->IsA<UStaticMeshComponent>() || Comp->IsA<USkeletalMeshComponent>())
		{
			TargetLoc = Comp->GetComponentLocation();
			break;
		}
	}

	if (LevitateVFX)
	{
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

	// 3. 물리 엔진을 이용해 강제로 위로 띄우기 설정
	if (ACharacter* TargetCharacter = Cast<ACharacter>(LevitatedTarget))
	{
		if (UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement())
		{
			MoveComp->GravityScale = 0.0f;                // 중력 무시
			MoveComp->SetMovementMode(MOVE_Flying);      // 낙하 방지
			MoveComp->Velocity = FVector(0.f, 0.f, 250.f); // 위로 살짝 띄우기
		}
	}
	else
	{
		// 사물(Platform)일 경우
		TArray<UPrimitiveComponent*> PrimitiveComps;
		LevitatedTarget->GetComponents<UPrimitiveComponent>(PrimitiveComps);
		for (UPrimitiveComponent* PrimComp : PrimitiveComps)
		{
			if (PrimComp->IsSimulatingPhysics())
			{
				HoverTargetComp = PrimComp;


				// 물리 시뮬레이션은 유지하되, 중력만 끄고 초기 속도를 위로 주어 살짝 띄우기
				PrimComp->SetSimulatePhysics(false);

				// 목표 높이 (현재 높이 + 250) 설정
				HoverTargetZ = PrimComp->GetComponentLocation().Z + 250.f;

				// 짧은 주기로 부양 로직으로 이동시킴
				GetWorld()->GetTimerManager().SetTimer(
					HoverTimerHandle,
					this,
					&UGA_Spell_Leviosa::UpdateHovering,
					0.01f,
					true
				);
				break;
			}
		}
	}

	Debug::Print(FString::Printf(TEXT("[Leviosa] Levitated %s!"), *LevitatedTarget->GetName()), FColor::Cyan);

	// 지정된 지속 시간(LevitationDuration) 이후에 어빌리티를 종료하도록 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(
		LevitationTimerHandle,
		this,
		&UGA_Spell_Leviosa::OnLevitationDurationEnded,
		LevitationDuration,
		false
	);

	return true;
}

void UGA_Spell_Leviosa::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// 대상이 없거나 맞아서 취소되었다면 그대로 마법 완전히 종료
	if (bInterrupted || !IsValid(LevitatedTarget))
	{
		GetWorld()->GetTimerManager().ClearTimer(LevitationTimerHandle);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
	}
	else
	{
		// 마법 시전 모션이 끝났다면, 
		// 어빌리티(마법 유지)는 끄지 않되, '캐스팅 중' 상태만 해제하여 플레이어가 바로 움직일 수 있게 풀어줌
		if (UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo())
		{
			OwnerASC->RemoveLooseGameplayTag(HOGGameplayTags::State_Casting_Active);
		}
	}
}

void UGA_Spell_Leviosa::OnLevitationDurationEnded()
{
	// 지속 시간 종료시 공중 부양 종료
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Spell_Leviosa::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 안전을 위해 종료될 때 타이머 무력화 
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LevitationTimerHandle);
		World->GetTimerManager().ClearTimer(HoverTimerHandle);
	}

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
		else if (HoverTargetComp)
		{
			// 꺼두었던 물리를 다시 켬

			HoverTargetComp->SetSimulatePhysics(true);
			HoverTargetComp->SetEnableGravity(true);
		}
        
        // 상태 초기화
        HoverTargetComp = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Spell_Leviosa::UpdateHovering()
{
	if (!IsValid(LevitatedTarget) || !IsValid(HoverTargetComp))
	{
		GetWorld()->GetTimerManager().ClearTimer(HoverTimerHandle);
		return;
	}

	// 현재 위치
	FVector CurrentLoc = HoverTargetComp->GetComponentLocation();

	// 목표 위치
	FVector TargetLoc = CurrentLoc;
	TargetLoc.Z = HoverTargetZ;

	// Z축으로 부드럽게 이동 (보간) - InterpSpeed(3.0f 등) 조절로 상승 속도 제어 가능
	// 델타 타임은 타이머 반복 주기 0.02f 사용
	FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLoc, 0.02f, 3.0f);

	// 컴포넌트의 위치를 강제로 이동 (물리가 꺼져있으므로 이대로 쭉 밀고 올라감 - 플레이어도 함께 밀려 올라감)
	HoverTargetComp->SetWorldLocation(NewLoc);

	// 만약 목표 높이에 거의 다 도달했으면 타이머를 끄고 멈추게 할 수도 있음
}