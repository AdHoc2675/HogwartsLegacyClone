#include "GAS/Abilities/Spell/Incendio/GA_Spell_Incendio.h"

#include "HOGDebugHelper.h"
#include "Data/DA_SpellDefinition.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/OverlapResult.h"

#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "NiagaraFunctionLibrary.h"

#include "Character/Player/PlayerCharacterBase.h"
#include "Components/StaticMeshComponent.h"

#include "NiagaraComponent.h"

#include "Core/HOG_GameplayTags.h"
#include "Core/HOG_Struct.h"
#include "Component/CombatComponent.h"
#include "Interactable/InteractableInterface.h"

#include "GameFramework/HOG_PlayerController.h"
#include "UI/HOG_WidgetController.h"

UGA_Spell_Incendio::UGA_Spell_Incendio()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Spell_Incendio::ActivateAbility(
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

	ExecuteIncendioCast(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Spell_Incendio::OnPreCastFacingFinished(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ExecuteIncendioCast(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Spell_Incendio::ExecuteIncendioCast(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{

	// Cast 시전 가능 여부 체크 (쿨다운, 자원 등). 실패 시 즉시 종료
	FSpellCastCheckResult CheckResult;
	if (!CanCastAsNormal(CheckResult))
	{
		const FSpellCastRequest FailedRequest = BuildSpellCastRequest(ESpellCastContext::Normal);
		NotifySpellCastFailedResult(FailedRequest, CheckResult);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());

	// 주문 시전 음성 재생
	if (CastVoiceSound && Character)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastVoiceSound, Character->GetActorLocation());

		// =============== [자막 호출] ===============
		if (AHOG_PlayerController* PC = Cast<AHOG_PlayerController>(Character->GetController()))
		{
			if (UHOG_WidgetController* UIController = PC->GetWidgetController())
			{
				UIController->RequestSubtitle(CastSubtitleText, 1.0f);
			}
		}
	}

	NotifySpellCastSucceeded(ESpellCastContext::Normal);

	if (!Character || !Character->GetMesh() || !CastMontage)
	{
		FireIncendio();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		FireIncendio();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	const float Duration = AnimInstance->Montage_Play(CastMontage, 3.f);
	if (Duration > 0.f)
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UGA_Spell_Incendio::OnMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, CastMontage);

		FireIncendio();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UGA_Spell_Incendio::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
}

void UGA_Spell_Incendio::FireIncendio()
{
	if (!CurrentActorInfo || !CurrentActorInfo->AvatarActor.IsValid())
	{
		return;
	}

	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
	UWorld* World = Avatar->GetWorld();
	if (!World)
	{
		return;
	}

	// 사거리 및 데미지 가져오기 (DA에서)
	const float Range = GetCastRange();
	const float BaseDmg = GetBaseDamage();

	if (Range <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return;
	}

	// ==============================
	// 1) 락온 / 에임 기준 방향 결정
	// ==============================
	FGameplayTagContainer TargetTags;
	FVector AimPoint = FVector::ZeroVector;
	AActor* LockedTarget = nullptr;

	const bool bLockedTargetUsed = TryConsumeLockedTarget(LockedTarget, TargetTags, AimPoint);

	const FVector AvatarLoc = Avatar->GetActorLocation();
	const FVector AvatarForward = Avatar->GetActorForwardVector().GetSafeNormal();
	const float ConeRange = (ConeRangeOverride > 0.f) ? ConeRangeOverride : Range;

	FVector ConeForward = AvatarForward;

	// 락온 대상이 있으면 대상을 그대로 사용
	if (bLockedTargetUsed && IsValid(LockedTarget))
	{
		const FVector TargetLoc = LockedTarget->GetActorLocation();
		ConeForward = (TargetLoc - AvatarLoc).GetSafeNormal();

		if (ConeForward.IsNearlyZero())
		{
			ConeForward = AvatarForward;
		}

		/*
		if (bDrawDebugLine)
		{
			Debug::Print(FString::Printf(
				TEXT("[Incendio] LockOn | Target=%s | AvatarLoc=%s | ConeForward=%s"),
				*GetNameSafe(LockedTarget),
				*AvatarLoc.ToString(),
				*ConeForward.ToString()
			));
		}
		*/
	}
	// 락온 대상이 없으면:
	// Pre-Cast Facing 단계에서 실제로 회전에 사용한 목표점을 우선 사용한다.
	else
	{
		FVector CachedFacingTarget = FVector::ZeroVector;
		bool bUseCachedFacingTarget = GetCachedPreCastFacingTargetLocation(CachedFacingTarget);

		if (!bUseCachedFacingTarget)
		{
			// 안전장치: 캐시가 없으면 기존 fallback aim point 사용
			bUseCachedFacingTarget = BuildFallbackAimPoint(CachedFacingTarget, ConeRange);
		}

		if (bUseCachedFacingTarget)
		{
			const FVector ToAim = (CachedFacingTarget - AvatarLoc).GetSafeNormal();

			if (!ToAim.IsNearlyZero())
			{
				ConeForward = ToAim;
			}
			else
			{
				ConeForward = AvatarForward;
			}
		}
		else
		{
			ConeForward = AvatarForward;
		}

		// Incendio는 전방 Cone 느낌 유지가 중요하므로 지면 기준으로 눕힌다.
		ConeForward = FVector(ConeForward.X, ConeForward.Y, 0.f).GetSafeNormal();

		if (ConeForward.IsNearlyZero())
		{
			ConeForward = AvatarForward;
		}

		/*
		if (bDrawDebugLine)
		{
			Debug::Print(FString::Printf(
				TEXT("[Incendio] NoLock | CachedFacingTarget=%s | AvatarLoc=%s | AvatarForward=%s | ConeForward=%s | UsedCached=%d"),
				*CachedFacingTarget.ToString(),
				*AvatarLoc.ToString(),
				*AvatarForward.ToString(),
				*ConeForward.ToString(),
				bUseCachedFacingTarget ? 1 : 0
			));
		}
		*/
	}

	// 원뿔 시작점을 캐릭터 바로 앞쪽으로 약간 이동
	const FVector ConeOrigin = AvatarLoc + (ConeForward * ConeStartOffset);

	// 총 부채꼴 각도는 ConeHalfAngleDeg * 2
	const float ConeHalfAngleRad = FMath::DegreesToRadians(ConeHalfAngleDeg);
	const float ConeMinDot = FMath::Cos(ConeHalfAngleRad);

	// ==============================
	// 2) 후보 수집용 구형 Overlap
	// ==============================
	FCollisionShape CandidateSphere = FCollisionShape::MakeSphere(ConeRange);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(IncendioConeAoE), false);

	if (bIgnoreSelf)
	{
		QueryParams.AddIgnoredActor(Avatar);
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	TArray<FOverlapResult> OverlapResults;
	const bool bHit = World->OverlapMultiByObjectType(
		OverlapResults,
		ConeOrigin,
		FQuat::Identity,
		ObjectQueryParams,
		CandidateSphere,
		QueryParams
	);

	// ==============================
	// 2-B) VFX 스폰 위치/회전 결정
	// ==============================
	const float VFXForwardOffset = 80.0f;
	const FVector VFXSpawnLocation = ConeOrigin + (ConeForward * VFXForwardOffset);
	const FRotator VFXSpawnRotation = ConeForward.Rotation();

	// ==============================
	// 3) VFX / SFX
	// ==============================
	if (FireVFX)
	{
		const FVector VFXScale(4.0f, 2.0f, 6.0f);

		UNiagaraComponent* SpawnedNiagara = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			FireVFX,
			VFXSpawnLocation,
			VFXSpawnRotation,
			VFXScale,
			true,
			true,
			ENCPoolMethod::None,
			true
		);

		ApplyIncendioVFXParams(
			SpawnedNiagara,
			ConeForward,
			ConeRange,
			ConeHalfAngleDeg
		);
	}

	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, ConeOrigin);
	}

	/*
	if (bDrawDebugLine)
	{
		const float DebugDuration = 2.0f;
		const FColor ConeColor = bHit ? FColor::Red : FColor::Green;

		// 정면 방향
		DrawDebugLine(
			World,
			ConeOrigin,
			ConeOrigin + (ConeForward * ConeRange),
			FColor::Orange,
			false,
			DebugDuration,
			0,
			2.0f
		);

		// 기준 축 계산
		const FVector RightVector = FVector::CrossProduct(FVector::UpVector, ConeForward).GetSafeNormal();
		const FVector UpVector = FVector::CrossProduct(ConeForward, RightVector).GetSafeNormal();

		if (!RightVector.IsNearlyZero() && !UpVector.IsNearlyZero())
		{
			// 좌/우 경계
			const FVector LeftBoundaryDir =
				(ConeForward * FMath::Cos(ConeHalfAngleRad) - RightVector * FMath::Sin(ConeHalfAngleRad)).GetSafeNormal();

			const FVector RightBoundaryDir =
				(ConeForward * FMath::Cos(ConeHalfAngleRad) + RightVector * FMath::Sin(ConeHalfAngleRad)).GetSafeNormal();

			// 상/하 경계
			const FVector UpBoundaryDir =
				(ConeForward * FMath::Cos(ConeHalfAngleRad) + UpVector * FMath::Sin(ConeHalfAngleRad)).GetSafeNormal();

			const FVector DownBoundaryDir =
				(ConeForward * FMath::Cos(ConeHalfAngleRad) - UpVector * FMath::Sin(ConeHalfAngleRad)).GetSafeNormal();

			// 좌/우 선
			DrawDebugLine(
				World,
				ConeOrigin,
				ConeOrigin + (LeftBoundaryDir * ConeRange),
				ConeColor,
				false,
				DebugDuration,
				0,
				2.0f
			);

			DrawDebugLine(
				World,
				ConeOrigin,
				ConeOrigin + (RightBoundaryDir * ConeRange),
				ConeColor,
				false,
				DebugDuration,
				0,
				2.0f
			);

			// 상/하 선
			DrawDebugLine(
				World,
				ConeOrigin,
				ConeOrigin + (UpBoundaryDir * ConeRange),
				FColor::Cyan,
				false,
				DebugDuration,
				0,
				2.0f
			);

			DrawDebugLine(
				World,
				ConeOrigin,
				ConeOrigin + (DownBoundaryDir * ConeRange),
				FColor::Cyan,
				false,
				DebugDuration,
				0,
				2.0f
			);

			// 수평 arc (좌 -> 우)
			{
				const int32 ArcSegments = 12;
				FVector PrevPoint = ConeOrigin + (LeftBoundaryDir * ConeRange);

				for (int32 i = 1; i <= ArcSegments; ++i)
				{
					const float T = static_cast<float>(i) / static_cast<float>(ArcSegments);
					const float Angle = FMath::Lerp(-ConeHalfAngleRad, ConeHalfAngleRad, T);

					const FVector ArcDir =
						(ConeForward * FMath::Cos(Angle) + RightVector * FMath::Sin(Angle)).GetSafeNormal();

					const FVector CurrPoint = ConeOrigin + (ArcDir * ConeRange);

					DrawDebugLine(
						World,
						PrevPoint,
						CurrPoint,
						ConeColor,
						false,
						DebugDuration,
						0,
						1.5f
					);

					PrevPoint = CurrPoint;
				}
			}

			// 수직 arc (하 -> 상)
			{
				const int32 ArcSegments = 12;
				FVector PrevPoint = ConeOrigin + (DownBoundaryDir * ConeRange);

				for (int32 i = 1; i <= ArcSegments; ++i)
				{
					const float T = static_cast<float>(i) / static_cast<float>(ArcSegments);
					const float Angle = FMath::Lerp(-ConeHalfAngleRad, ConeHalfAngleRad, T);

					const FVector ArcDir =
						(ConeForward * FMath::Cos(Angle) + UpVector * FMath::Sin(Angle)).GetSafeNormal();

					const FVector CurrPoint = ConeOrigin + (ArcDir * ConeRange);

					DrawDebugLine(
						World,
						PrevPoint,
						CurrPoint,
						FColor::Cyan,
						false,
						DebugDuration,
						0,
						1.5f
					);

					PrevPoint = CurrPoint;
				}
			}
		}
	}
	*/

	if (!bHit)
	{
		return;
	}

	// ==============================
	// 4) 후보 필터 + 처리
	// ==============================
	TSet<AActor*> HitActors;

	for (const FOverlapResult& Overlap : OverlapResults)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!TargetActor || HitActors.Contains(TargetActor))
		{
			continue;
		}

		HitActors.Add(TargetActor);

		const FVector TargetPoint = TargetActor->GetActorLocation();
		const FVector ToTarget = TargetPoint - ConeOrigin;

		const float DistToTarget = ToTarget.Size();
		if (DistToTarget > ConeRange)
		{
			continue;
		}

		FVector ToTargetDir = ToTarget.GetSafeNormal();
		if (ToTargetDir.IsNearlyZero())
		{
			ToTargetDir = ConeForward;
		}

		const float Dot = FVector::DotProduct(ConeForward, ToTargetDir);
		if (Dot < ConeMinDot)
		{
			continue;
		}

		// ==============================
		// 4-1) 인터랙터블 대상이면 Burn 태그 신호 전달
		// ==============================
		if (TargetActor->Implements<UInteractableInterface>())
		{
			const FGameplayTag BurnInteractionTag = HOGGameplayTags::Interaction_Burn;

			const bool bCanReceiveBurn = IInteractableInterface::Execute_CanReceiveInteractionTag(
				TargetActor,
				Avatar,
				BurnInteractionTag
			);

			if (bCanReceiveBurn)
			{
				IInteractableInterface::Execute_ReceiveInteractionTag(TargetActor, Avatar, BurnInteractionTag);
			}
		}

		// ==============================
		// 4-2) 전투 대상 유효성 검사
		// ==============================
		const bool bMeetsRequirement = DoesTargetMeetRequirements(TargetActor);
		if (!bMeetsRequirement)
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = nullptr;
		FGameplayTagContainer ActualTargetTags;

		if (TargetActor->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
		{
			if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor))
			{
				TargetASC = ASI->GetAbilitySystemComponent();
			}
		}

		if (!TargetASC)
		{
			TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
		}

		if (TargetASC)
		{
			TargetASC->GetOwnedGameplayTags(ActualTargetTags);
		}

		UCombatComponent* CombatComp = TargetActor->FindComponentByClass<UCombatComponent>();
		if (!CombatComp)
		{
			continue;
		}

		// ==============================
		// 4-3) 즉발 데미지
		// ==============================
		FDamageRequest DamageRequest;
		DamageRequest.SourceActor = Avatar;
		DamageRequest.TargetActor = TargetActor;
		DamageRequest.InstigatorActor = Avatar;
		DamageRequest.DamageCauser = Avatar;
		DamageRequest.BaseDamage = BaseDmg;
		DamageRequest.SourceTags = FGameplayTagContainer();
		DamageRequest.TargetTags = ActualTargetTags;

		FDamageResult DamageResult = CombatComp->ApplyDamageRequest(DamageRequest);

		if (!DamageResult.bWasApplied)
		{
			continue;
		}

		// ==============================
		// 4-4) Burn 상태이상 적용
		// ==============================
		if (TargetASC && DotDamageEffectClass)
		{
			const float BurnDamagePerTick = BaseDmg * 0.3f;

			FGameplayEffectSpecHandle DotSpec = MakeOutgoingGameplayEffectSpec(DotDamageEffectClass, 1.0f);
			if (DotSpec.IsValid())
			{
				DotSpec.Data->SetSetByCallerMagnitude(HOGGameplayTags::Data_Damage, BurnDamagePerTick);
				SourceASC->ApplyGameplayEffectSpecToTarget(*DotSpec.Data.Get(), TargetASC);
			}
		}
	}
}

void UGA_Spell_Incendio::ApplyIncendioVFXParams(
	UNiagaraComponent* NiagaraComp,
	const FVector& InConeForward,
	float InConeRange,
	float InConeHalfAngleDeg
) const
{
	if (!NiagaraComp)
	{
		return;
	}

	const FVector SafeForward = InConeForward.GetSafeNormal();

	const float HalfAngleRad = FMath::DegreesToRadians(InConeHalfAngleDeg);
	const float LateralSpread = InConeRange * FMath::Tan(HalfAngleRad) * 0.35f;

	const FVector MinimumVelocity(InConeRange * 0.35f, -LateralSpread, 0.0f);
	const FVector MaximumVelocity(InConeRange * 0.60f, LateralSpread, InConeRange * 0.04f);
	
	const float RingRadius = FMath::Clamp(LateralSpread * 0.35f, 12.0f, 120.0f);
	NiagaraComp->SetVariableFloat(TEXT("User.RingRadius"), RingRadius);

	NiagaraComp->SetVariableVec3(TEXT("User.ConeForward"), SafeForward);
	NiagaraComp->SetVariableFloat(TEXT("User.ConeRange"), InConeRange);
	NiagaraComp->SetVariableFloat(TEXT("User.ConeHalfAngleDeg"), InConeHalfAngleDeg);

	NiagaraComp->SetVariableVec3(TEXT("User.MinimumVelocity"), MinimumVelocity);
	NiagaraComp->SetVariableVec3(TEXT("User.MaximumVelocity"), MaximumVelocity);
}