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

#include "Core/HOG_GameplayTags.h"
#include "Core/HOG_Struct.h"
#include "Component/CombatComponent.h"
#include "Interactable/InteractableInterface.h"

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

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		Debug::Print(TEXT("[Incendio] CommitAbility Failed"), FColor::Red);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());

	// 주문 시전 음성 재생
	if (CastVoiceSound && Character)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastVoiceSound, Character->GetActorLocation());
	}

	if (!Character || !Character->GetMesh() || !CastMontage)
	{
		Debug::Print(TEXT("[Incendio] No Character/Mesh/Montage -> FireIncendio direct"));
		FireIncendio();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		Debug::Print(TEXT("[Incendio] AnimInstance Missing -> FireIncendio direct"));
		FireIncendio();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	const float Duration = AnimInstance->Montage_Play(CastMontage, 1.f);
	if (Duration > 0.f)
	{
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &UGA_Spell_Incendio::OnMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, CastMontage);

		Debug::Print(TEXT("[Incendio] Montage Play Success -> FireIncendio"));
		FireIncendio();
	}
	else
	{
		Debug::Print(TEXT("[Incendio] Montage Play Failed"), FColor::Red);
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
		Debug::Print(TEXT("[Incendio] FireIncendio Abort | CurrentActorInfo Invalid"), FColor::Red);
		return;
	}

	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
	UWorld* World = Avatar->GetWorld();
	if (!World)
	{
		Debug::Print(TEXT("[Incendio] FireIncendio Abort | World Invalid"), FColor::Red);
		return;
	}

	// 사거리 및 데미지 가져오기 (DA에서)
	const float Range = GetCastRange();
	const float BaseDmg = GetBaseDamage();

	Debug::Print(
		FString::Printf(
			TEXT("[Incendio] Fire Start | Avatar=%s | Range=%.2f | BaseDmg=%.2f"),
			*GetNameSafe(Avatar),
			Range,
			BaseDmg
		)
	);

	if (Range <= 0.f)
	{
		Debug::Print(TEXT("[Incendio] Range is 0! Check SpellDefinition Data Asset."), FColor::Red);
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		Debug::Print(TEXT("[Incendio] SourceASC is null."), FColor::Red);
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

	FVector TargetLoc = IsValid(LockedTarget)
		                    ? LockedTarget->GetActorLocation()
		                    : AimPoint;

	const float ConeRange = (ConeRangeOverride > 0.f) ? ConeRangeOverride : Range;

	if (TargetLoc.IsNearlyZero())
	{
		TargetLoc = AvatarLoc + (Avatar->GetActorForwardVector() * ConeRange);
		Debug::Print(TEXT("[Incendio] TargetLoc fallback to ForwardVector"));
	}

	FVector ConeForward = (TargetLoc - AvatarLoc).GetSafeNormal();
	if (ConeForward.IsNearlyZero())
	{
		ConeForward = Avatar->GetActorForwardVector();
	}


	// 원뿔 시작점을 캐릭터 바로 앞쪽으로 약간 이동
	const FVector ConeOrigin = AvatarLoc + (ConeForward * ConeStartOffset);

	// 총 부채꼴 각도는 ConeHalfAngleDeg * 2
	const float ConeHalfAngleRad = FMath::DegreesToRadians(ConeHalfAngleDeg);
	const float ConeMinDot = FMath::Cos(ConeHalfAngleRad);

	Debug::Print(
		FString::Printf(
			TEXT(
				"[Incendio] Cone Setup | UsedLocked=%d | LockedTarget=%s | ConeOrigin=%s | Forward=%s | ConeRange=%.2f | HalfAngle=%.2f"),
			bLockedTargetUsed ? 1 : 0,
			*GetNameSafe(LockedTarget),
			*ConeOrigin.ToString(),
			*ConeForward.ToString(),
			ConeRange,
			ConeHalfAngleDeg
		)
	);
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

	Debug::Print(
		FString::Printf(
			TEXT("[Incendio] Overlap Result | bHit=%d | Count=%d"),
			bHit ? 1 : 0,
			OverlapResults.Num()
		)
	);

	// ==============================
	// 3) VFX / SFX
	// ==============================
	if (FireVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, FireVFX, ConeOrigin, ConeForward.Rotation());
	}

	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, ConeOrigin);
	}

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
				(ConeForward * FMath::Cos(ConeHalfAngleRad) - RightVector * FMath::Sin(ConeHalfAngleRad)).
				GetSafeNormal();

			const FVector RightBoundaryDir =
				(ConeForward * FMath::Cos(ConeHalfAngleRad) + RightVector * FMath::Sin(ConeHalfAngleRad)).
				GetSafeNormal();

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

	if (!bHit)
	{
		Debug::Print(TEXT("[Incendio] No overlap candidates."), FColor::Yellow);
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
			Debug::Print(
				FString::Printf(
					TEXT("[Incendio] Cone Reject Distance | Target=%s | Dist=%.2f"),
					*GetNameSafe(TargetActor),
					DistToTarget
				)
			);
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
			Debug::Print(
				FString::Printf(
					TEXT("[Incendio] Cone Reject Angle | Target=%s | Dot=%.3f | Need>=%.3f"),
					*GetNameSafe(TargetActor),
					Dot,
					ConeMinDot
				)
			);
			continue;
		}

		Debug::Print(
			FString::Printf(
				TEXT("[Incendio] Cone Accept | Target=%s | Dist=%.2f | Dot=%.3f"),
				*GetNameSafe(TargetActor),
				DistToTarget,
				Dot
			)
		);

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

			Debug::Print(
				FString::Printf(
					TEXT("[Incendio] Interactable Check | Target=%s | CanReceiveBurn=%d"),
					*GetNameSafe(TargetActor),
					bCanReceiveBurn ? 1 : 0
				)
			);

			if (bCanReceiveBurn)
			{
				IInteractableInterface::Execute_ReceiveInteractionTag(TargetActor, Avatar, BurnInteractionTag);
				Debug::Print(
					FString::Printf(
						TEXT("[Incendio] Burn Interaction Sent | Target=%s"),
						*GetNameSafe(TargetActor)
					)
				);
			}
		}

		// ==============================
		// 4-2) 전투 대상 유효성 검사
		// ==============================
		const bool bMeetsRequirement = DoesTargetMeetRequirements(TargetActor);
		if (!bMeetsRequirement)
		{
			Debug::Print(
				FString::Printf(
					TEXT("[Incendio] Requirement Failed | Target=%s"),
					*GetNameSafe(TargetActor)
				)
			);
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

			Debug::Print(
				FString::Printf(
					TEXT("[Incendio] TargetASC Found | Target=%s | Tags=%s"),
					*GetNameSafe(TargetActor),
					*ActualTargetTags.ToStringSimple()
				)
			);
		}
		else
		{
			Debug::Print(
				FString::Printf(
					TEXT("[Incendio] TargetASC Missing | Target=%s"),
					*GetNameSafe(TargetActor)
				)
			);
		}

		UCombatComponent* CombatComp = TargetActor->FindComponentByClass<UCombatComponent>();
		if (!CombatComp)
		{
			Debug::Print(
				FString::Printf(
					TEXT("[Incendio] CombatComponent Missing -> Skip | Target=%s"),
					*GetNameSafe(TargetActor)
				)
			);
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

		Debug::Print(
			FString::Printf(
				TEXT("[Incendio] DamageApplied=%d | Target=%s"),
				DamageResult.bWasApplied ? 1 : 0,
				*GetNameSafe(TargetActor)
			)
		);

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

				Debug::Print(
					FString::Printf(
						TEXT("[Incendio] BurnApplied | Target=%s | BurnPerTick=%.2f"),
						*GetNameSafe(TargetActor),
						BurnDamagePerTick
					)
				);
			}
			else
			{
				Debug::Print(
					FString::Printf(
						TEXT("[Incendio] Burn Spec Invalid | Target=%s"),
						*GetNameSafe(TargetActor)
					)
				);
			}
		}
		else
		{
			Debug::Print(
				FString::Printf(
					TEXT("[Incendio] Burn Skip | Target=%s | HasASC=%d | HasBurnGE=%d"),
					*GetNameSafe(TargetActor),
					TargetASC ? 1 : 0,
					DotDamageEffectClass ? 1 : 0
				)
			);
		}
	}
}
