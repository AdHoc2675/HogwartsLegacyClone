#include "GAS/Abilities/Spell/Leviosa/GA_Spell_Leviosa.h"
#include "HOGDebugHelper.h"
#include "Core/HOG_GameplayTags.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Interactable/InteractableLevitatable.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/HOG_PlayerController.h"
#include "UI/HOG_WidgetController.h"

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
	
	FGameplayTagContainer RelevantTags;
	if (!CheckCooldown(Handle, ActorInfo, &RelevantTags))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (TryBeginPreCastFacing(Handle, ActorInfo, ActivationInfo, TriggerEventData))
	{
		return;
	}

	ExecuteLeviosaCast(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Spell_Leviosa::OnPreCastFacingFinished(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ExecuteLeviosaCast(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Spell_Leviosa::ExecuteLeviosaCast(
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

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());

	NotifySpellCastSucceeded(ESpellCastContext::Normal);

	// 1. 주문 시전 사운드 재생
	if (CastVoiceSound && Character)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastVoiceSound, Character->GetActorLocation());
		
		// ===== [자막 호출] =====
		if (AHOG_PlayerController* PC = Cast<AHOG_PlayerController>(Character->GetController()))
		{
			// PlayerController에 GetWidgetController() 함수가 있다고 가정 (프로젝트에 맞게 호출)
			if (UHOG_WidgetController* UIController = PC->GetWidgetController())
			{
				UIController->RequestSubtitle(CastSubtitleText, 1.0f); // 2초 유지
			}
		}
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
		//Debug::Print(FString::Printf(TEXT("[Leviosa] Target Acquired: %s"), *LevitatedTarget->GetName()), FColor::Cyan);
	}
	else
	{
		//Debug::Print(TEXT("[Leviosa] No valid Target. (Casting empty)"), FColor::Yellow);
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
		//Debug::Print(TEXT("[Leviosa] No valid target to levitate."), FColor::Yellow);
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
	
	// 기본값은 어빌리티에 설정된 값을 사용
	float FinalRiseDuration = LevitationDuration;
	float FinalHoverDuration = LevitationHoverDuration; // 추가된 체공 시간 초기화
	float FinalLevitateZOffset = 250.f;

	// 대상이 InteractableLevitatable라면 개별 설정된 값을 덮어씌움
	if (AInteractableLevitatable* LevitatableObj = Cast<AInteractableLevitatable>(LevitatedTarget))
	{
		FinalRiseDuration = LevitatableObj->GetLevitateDuration();
		FinalLevitateZOffset = LevitatableObj->GetLevitateHeight();
		FinalHoverDuration = LevitatableObj->GetLevitateHoverDuration(); // 체공 시간 덮어쓰기
	}

	if (ACharacter* TargetCharacter = Cast<ACharacter>(LevitatedTarget))
	{
		if (UCharacterMovementComponent* MoveComp = TargetCharacter->GetCharacterMovement())
		{
			MoveComp->GravityScale = 0.0f;
			MoveComp->SetMovementMode(MOVE_Flying);
			MoveComp->Velocity = FVector::ZeroVector; // 기존 이동 속도 상쇄

			MoveComp->DisableMovement();
			TargetCharacter->GetCapsuleComponent()->SetPhysicsLinearVelocity(FVector::ZeroVector);

		}
		
		// 애니메이션 루트모션 이동 방지를 위해 타겟 컴포넌트 대신 캐릭터 자체를 타겟팅
		HoverTargetComp = TargetCharacter->GetCapsuleComponent(); 
		HoverInitialZ = TargetCharacter->GetActorLocation().Z;
		HoverTargetZ = HoverInitialZ + FinalLevitateZOffset;
		
		HoverElapsedTime = 0.0f;
		CurrentLevitateDuration = FMath::Max(0.1f, FinalRiseDuration);
		
		// 띄우는 중에는 AI/이동 방해 막기 위해 타이머 돌림
		GetWorld()->GetTimerManager().SetTimer(HoverTimerHandle, this, &UGA_Spell_Leviosa::UpdateHovering, 0.01f, true);
	}
	else
	{
		// 사물(Platform)일 경우 기존 로직 유지
		TArray<UPrimitiveComponent*> PrimitiveComps;
		LevitatedTarget->GetComponents<UPrimitiveComponent>(PrimitiveComps);
		for (UPrimitiveComponent* PrimComp : PrimitiveComps)
		{
			if (PrimComp->IsSimulatingPhysics())
			{
				HoverTargetComp = PrimComp;

				// 물리 시뮬레이션은 유지하되, 중력만 끄고 초기 속도를 위로 주어 살짝 띄우기
				PrimComp->SetSimulatePhysics(false);

				// 목표 높이 설정
				HoverInitialZ = PrimComp->GetComponentLocation().Z;
				HoverTargetZ = HoverInitialZ + FinalLevitateZOffset;
				
				// 보간을 위한 시간 초기화
				HoverElapsedTime = 0.0f;
				CurrentLevitateDuration = FMath::Max(0.1f, FinalRiseDuration); // 시간 0 방어 코드

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

	//Debug::Print(FString::Printf(TEXT("[Leviosa] Levitated %s!"), *LevitatedTarget->GetName()), FColor::Cyan);

	// 부유가 끝난 직후 + 체공 시간(HoverDuration)을 모두 버틴 뒤 어빌리티를 종료하도록 함
	float TotalLevitationTime = FinalRiseDuration + FinalHoverDuration;

	GetWorld()->GetTimerManager().SetTimer(
		LevitationTimerHandle,
		this,
		&UGA_Spell_Leviosa::OnLevitationDurationEnded,
		TotalLevitationTime, 
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
        
				// 꺼두었던 이동 연산을 다시 활성화 (걷기 상태로 초기화)
				MoveComp->SetMovementMode(MOVE_Walking); 

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
		LevitatedTarget = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Spell_Leviosa::UpdateHovering()
{
	if (!IsValid(LevitatedTarget) || (!IsValid(HoverTargetComp) && !LevitatedTarget->IsA<ACharacter>()))
	{
		GetWorld()->GetTimerManager().ClearTimer(HoverTimerHandle);
		return;
	}

	HoverElapsedTime += 0.01f;
	float Alpha = FMath::Clamp(HoverElapsedTime / CurrentLevitateDuration, 0.0f, 1.0f);
	float EasedAlpha = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);
	float NewZ = FMath::Lerp(HoverInitialZ, HoverTargetZ, EasedAlpha);

	if (ACharacter* TargetCharacter = Cast<ACharacter>(LevitatedTarget))
	{
		FVector CurrentLoc = TargetCharacter->GetActorLocation();
		CurrentLoc.Z = NewZ;
		TargetCharacter->SetActorLocation(CurrentLoc, false); // Sweep 방벽 무시 강제 적용
	}
	else if (HoverTargetComp)
	{
		FVector CurrentLoc = HoverTargetComp->GetComponentLocation();
		CurrentLoc.Z = NewZ;
		HoverTargetComp->SetWorldLocation(CurrentLoc); // 사물은 기존 유지
	}

	if (Alpha >= 1.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(HoverTimerHandle);
	}
}