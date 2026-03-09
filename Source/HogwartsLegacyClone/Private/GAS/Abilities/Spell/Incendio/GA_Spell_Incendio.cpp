// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/Spell/Incendio/GA_Spell_Incendio.h"
#include "HOGDebugHelper.h"
#include "Data/DA_SpellDefinition.h"

#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"

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
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character || !Character->GetMesh() || !CastMontage)
	{
		// 몽타주가 없으면 즉시 발사 후 종료
		FireIncendio();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		const float Duration = AnimInstance->Montage_Play(CastMontage, 1.f);
		if (Duration > 0.f)
		{
			FOnMontageEnded EndDelegate;
			EndDelegate.BindUObject(this, &UGA_Spell_Incendio::OnMontageEnded);
			AnimInstance->Montage_SetEndDelegate(EndDelegate, CastMontage);

			// TODO: 애니메이션 노티파이(AnimNotify)를 사용해 특정 프레임에서 FireIncendio()를 호출하는 것이 정석
			// 현재는 몽타주 시작과 동시에 타격을 판정
			FireIncendio();
		}
		else
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		}
	}
}

void UGA_Spell_Incendio::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
}

void UGA_Spell_Incendio::FireIncendio()
{
	if (!CurrentActorInfo || !CurrentActorInfo->AvatarActor.IsValid()) return;

	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
	UWorld* World = Avatar->GetWorld();
	if (!World) return;

	// 사거리 및 데미지 가져오기 (DA에서)
	const float Range = GetCastRange();
	const float BaseDmg = GetBaseDamage();

	// 트레이스 시작 / 끝 지점 (아바타 전방으로 두꺼운 구형 캐스팅)
	FVector StartLoc = Avatar->GetActorLocation();
	FVector EndLoc = StartLoc + (Avatar->GetActorForwardVector() * Range);

	FCollisionShape SphereShape = FCollisionShape::MakeSphere(AttackRadius);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(IncendioTrace), false, Avatar);

	TArray<FHitResult> HitResults;
	bool bHit = World->SweepMultiByChannel(
		HitResults,
		StartLoc,
		EndLoc,
		FQuat::Identity,
		TraceChannel,
		SphereShape,
		QueryParams
	);

	// 시각 효과 재생
	if (FireVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, FireVFX, StartLoc + (Avatar->GetActorForwardVector() * 100.f), Avatar->GetActorRotation());
	}

	// 디버그 드로우
	if (bDrawDebug)
	{
		FColor DebugColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugCapsule(World, StartLoc + (EndLoc - StartLoc) * 0.5f, Range * 0.5f, AttackRadius, FRotationMatrix::MakeFromZ(EndLoc - StartLoc).ToQuat(), DebugColor, false, 2.0f);
	}

	if (!bHit) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	// 중복 타격 방지용 히트 액터 세트
	TSet<AActor*> HitActors;

	for (const FHitResult& Hit : HitResults)
	{
		AActor* TargetActor = Hit.GetActor();
		if (!TargetActor || HitActors.Contains(TargetActor)) continue;

		HitActors.Add(TargetActor);

		// 베이스 클래스의 공통 타겟 유효성 검사 (태그 기반 방어막 등 체크)
		if (!DoesTargetMeetRequirements(TargetActor)) continue;

		// 타겟의 ASC 가져오기
		UAbilitySystemComponent* TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
		if (TargetASC)
		{
			// 1. 즉발 데미지 (폭발)
			if (InstantDamageEffectClass)
			{
				FGameplayEffectSpecHandle InstantSpec = MakeOutgoingGameplayEffectSpec(InstantDamageEffectClass, 1.0f);
				if (InstantSpec.IsValid())
				{
					// 필요에 따라 데미지를 동적으로 할당 (SetByCaller)
					// InstantSpec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), BaseDmg);
					ASC->ApplyGameplayEffectSpecToTarget(*InstantSpec.Data.Get(), TargetASC);
				}
			}

			// 2. 화상 부여 (Damage over Time)
			if (DotDamageEffectClass)
			{
				FGameplayEffectSpecHandle DotSpec = MakeOutgoingGameplayEffectSpec(DotDamageEffectClass, 1.0f);
				if (DotSpec.IsValid())
				{
					ASC->ApplyGameplayEffectSpecToTarget(*DotSpec.Data.Get(), TargetASC);
				}
			}

			Debug::Print(FString::Printf(TEXT("[Incendio] Hit -> %s, Dmg: %.1f"), *TargetActor->GetName(), BaseDmg));
		}
	}
}