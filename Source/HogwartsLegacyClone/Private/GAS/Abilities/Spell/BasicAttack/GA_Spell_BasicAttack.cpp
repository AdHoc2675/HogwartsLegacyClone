// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/Spell/BasicAttack/GA_Spell_BasicAttack.h"

#include "HOGDebugHelper.h"
#include "Data/DA_SpellDefinition.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
#include "AbilitySystemComponent.h"

UGA_Spell_BasicAttack::UGA_Spell_BasicAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Spell_BasicAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 커밋 단계에서는 CanActivate 검증을 더 넣어도 되지만,
	// 지금은 “추가만”으로 최소 실행 경로만 보장.
	PlayComboMontageOrFire(ActorInfo);
}

void UGA_Spell_BasicAttack::PlayComboMontageOrFire(const FGameplayAbilityActorInfo* ActorInfo)
{
	ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;

	// 몽타주가 없거나 캐릭터/애님이 없으면 즉시 발사
	if (ComboMontages.Num() == 0 || !Character || !Character->GetMesh())
	{
		FireHitScan(ActorInfo);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		FireHitScan(ActorInfo);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 유효한 몽타주 찾기
	int32 SafeIndex = ComboIndex % ComboMontages.Num();
	UAnimMontage* MontageToPlay = ComboMontages[SafeIndex].Get();

	if (!MontageToPlay)
	{
		FireHitScan(ActorInfo);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	ComboIndex = (ComboIndex + 1) % ComboMontages.Num();

	// 몽타주 재생
	const float PlayRate = 1.f;
	const float Duration = AnimInstance->Montage_Play(MontageToPlay, PlayRate);

	if (Duration <= 0.f)
	{
		FireHitScan(ActorInfo);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 몽타주 종료 콜백
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UGA_Spell_BasicAttack::OnMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, MontageToPlay);

	// “발사 타이밍”을 몽타주 노티파이로 정밀하게 하고 싶으면 다음 단계에서 처리.
	// 현재는 간단 버전: 몽타주 시작 즉시 발사
	FireHitScan(ActorInfo);
}

void UGA_Spell_BasicAttack::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
}

bool UGA_Spell_BasicAttack::BuildTraceStartEnd(const FGameplayAbilityActorInfo* ActorInfo, FVector& OutStart, FVector& OutEnd, AActor*& OutLockTarget) const
{
	OutLockTarget = nullptr;

	// 사거리: Definition 기반
	const float Range = GetCastRange();
	if (Range <= 0.f)
	{
		return false;
	}

	// LockOn 기반 타겟/조준점 획득
	FGameplayTagContainer TargetTags;
	FVector AimPoint;
	AActor* Target = nullptr;

	AcquireTargetFromLockOn(Target, TargetTags, AimPoint);

	// 시작점: 카메라 센터에임 기준(SpellBase의 센터 계산 사용)
	FVector CenterAim;
	if (!GetCenterAimPoint(CenterAim, Range))
	{
		return false;
	}

	// CenterAimPoint는 “카메라에서 Range만큼 나간 끝점”이라 Start가 필요함.
	// Start는 카메라 위치로 잡는 게 깔끔하지만, SpellBase는 End만 제공하므로
	// 여기서는 End에서 역산하지 말고, Actor 위치를 Start로 잡는 간단 버전으로 간다.
	// (다음 단계에서 Start를 카메라 위치로 반환하도록 SpellBase 확장 가능)
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar)
	{
		return false;
	}

	OutStart = Avatar->GetActorLocation();

	// 끝점: 타겟이 있으면 타겟 위치, 없으면 AimPoint(또는 CenterAim)
	if (IsValid(Target))
	{
		OutLockTarget = Target;
		OutEnd = Target->GetActorLocation();
	}
	else
	{
		OutEnd = AimPoint.IsNearlyZero() ? CenterAim : AimPoint;
	}

	return true;
}

void UGA_Spell_BasicAttack::FireHitScan(const FGameplayAbilityActorInfo* ActorInfo)
{
	UWorld* World = GetWorld();
	if (!World || !ActorInfo)
	{
		return;
	}

	AActor* Avatar = ActorInfo->AvatarActor.Get();
	if (!Avatar)
	{
		return;
	}

	FVector Start, End;
	AActor* LockTarget = nullptr;

	if (!BuildTraceStartEnd(ActorInfo, Start, End, LockTarget))
	{
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(HOG_BasicAttackTrace), false);
	if (bIgnoreSelf)
	{
		Params.AddIgnoredActor(Avatar);
	}

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params);

	if (bDrawDebugLine)
	{
		const FVector DebugEnd = bHit ? Hit.ImpactPoint : End;
		DrawDebugLine(World, Start, DebugEnd, bHit ? FColor::Red : FColor::Green, false, 1.0f, 0, 2.0f);
	}

	// 데미지 수치: Definition 기반
	const float Damage = GetBaseDamage();

	if (bHit && Hit.GetActor())
	{
		// 여기서 실제 Damage 적용 파이프라인은 “CombatComponent/ExecutionCalculation” 쪽으로 갈 예정이므로,
		// 지금은 디버그만 찍고, 다음 단계에서 ApplyDamage GE/Execution으로 교체.
		Debug::Print(FString::Printf(TEXT("[BasicAttack] Hit=%s Damage=%.1f"), *GetNameSafe(Hit.GetActor()), Damage));
	}
	else
	{
		Debug::Print(TEXT("[BasicAttack] No Hit"));
	}
}
