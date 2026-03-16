// Fill out your copyright notice in the Description page of Project Settings.

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
#include "NiagaraFunctionLibrary.h"
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
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	
	// 🟢 주문 시전 음성 재생 (캐릭터 위치에서)
	if (CastVoiceSound && Character)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastVoiceSound, Character->GetActorLocation());
	}

	if (!Character || !Character->GetMesh() || !CastMontage)
	{
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

			// 지금은 몽타주 시작과 동시에 타격을 판정
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

	if (Range <= 0.f)
	{
		Debug::Print(TEXT("[Incendio] Range is 0! Check SpellDefinition Data Asset."), FColor::Red);
		return;
	}

	// 1. 락온 및 조준점 획득
	FGameplayTagContainer TargetTags;
	FVector AimPoint;
	AActor* Target = nullptr;

	TryConsumeLockedTarget(Target, TargetTags, AimPoint);

	FVector StartLoc = Avatar->GetActorLocation();
	// 타겟이 있으면 타겟 위치, 없으면 카메라 에임 포인트
	FVector TargetLoc = IsValid(Target) ? Target->GetActorLocation() : AimPoint;

	// 방향 벡터 계산 (VFX 회전용 및 최대 사거리 제한용)
	FVector Direction = (TargetLoc - StartLoc).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = Avatar->GetActorForwardVector();
	}

	// 2. 폭발 중심점(ExplosionCenter) 결정
	// 목표 위치가 최대 사거리보다 멀다면 최대 사거리까지만 폭발 지점 이동
	FVector ExplosionCenter = TargetLoc;
	const float DistanceToTarget = FVector::Dist(StartLoc, TargetLoc);
	if (DistanceToTarget > Range)
	{
		ExplosionCenter = StartLoc + (Direction * Range);
	}

	// 3. 목표 지점을 중심으로 한 구형(Sphere) Overlap 검사
	FCollisionShape SphereShape = FCollisionShape::MakeSphere(AttackRadius);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(IncendioAoE), false);

	if (bIgnoreSelf)
	{
		QueryParams.AddIgnoredActor(Avatar);
	}

	TArray<FOverlapResult> OverlapResults;
	bool bHit = World->OverlapMultiByChannel(
		OverlapResults,
		ExplosionCenter,
		FQuat::Identity,
		TraceChannel,
		SphereShape,
		QueryParams
	);

	// 시각 효과 재생 (캐릭터 위치가 아닌 폭발 중심점에 스폰)
	if (FireVFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, FireVFX, ExplosionCenter, Direction.Rotation());
	}

	// 🟢 화염 폭발 효과음 재생 (폭발 위치에서)
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, ExplosionCenter);
	}

	// 디버그 드로우 (어떻게 판정되는지 '구' 형태로 시각화)
	if (bDrawDebugLine)
	{
		FColor DebugColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugSphere(World, ExplosionCenter, AttackRadius, 32, DebugColor, false, 2.0f);
	}

	if (!bHit) {
		Debug::Print(TEXT("[Incendio] No hit detected in explosion area."), FColor::Yellow);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	// 중복 타격 방지용 액터 세트
	TSet<AActor*> HitActors;

	for (const FOverlapResult& Overlap : OverlapResults)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!TargetActor || HitActors.Contains(TargetActor)) continue;

		HitActors.Add(TargetActor);

		// 타겟이 상호작용 가능한 객체라면 Interact 호출
		if (TargetActor->Implements<UInteractableInterface>())
		{
			// IInteractableInterface에 정의된 함수명에 맞게 호출
			IInteractableInterface::Execute_Interact(TargetActor, Avatar);
		}

		// 태그 기반 방어막/면역 등 공통 타겟 유효성 검사
		if (!DoesTargetMeetRequirements(TargetActor)) continue;

		// 타겟의 ASC 가져와서 효과 적용
		UAbilitySystemComponent* TargetASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
		if (TargetASC)
		{
			// 즉발 데미지
			if (InstantDamageEffectClass)
			{
				FGameplayEffectSpecHandle InstantSpec = MakeOutgoingGameplayEffectSpec(InstantDamageEffectClass, 1.0f);
				if (InstantSpec.IsValid())
				{
					ASC->ApplyGameplayEffectSpecToTarget(*InstantSpec.Data.Get(), TargetASC);
				}
			}

			// 화상 부여 (도트 데미지)
			if (DotDamageEffectClass)
			{
				FGameplayEffectSpecHandle DotSpec = MakeOutgoingGameplayEffectSpec(DotDamageEffectClass, 1.0f);
				if (DotSpec.IsValid())
				{
					ASC->ApplyGameplayEffectSpecToTarget(*DotSpec.Data.Get(), TargetASC);
				}
			}

			// Debug::Print(FString::Printf(TEXT("[Incendio] Hit -> %s, Dmg: %.1f"), *TargetActor->GetName(), BaseDmg));
		}
	}
}