// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/Spell/Leviosa/GA_Spell_Leviosa.h"
#include "HOGDebugHelper.h"

#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"

UGA_Spell_Leviosa::UGA_Spell_Leviosa()
{
	// 타겟 상태를 기록하고 유지해야 하므로 Instanced로 설정
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Spell_Leviosa::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 자원/쿨다운 소모 처리
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1. 타겟 획득 (SpellBase의 LockOn 기능 활용)
	FGameplayTagContainer TargetTags;
	FVector AimPoint;
	AActor* AcquiredTarget = nullptr;

	bool bHasTarget = AcquireTargetFromLockOn(AcquiredTarget, TargetTags, AimPoint);

	if (bHasTarget && IsValid(AcquiredTarget))
	{
		LevitatedTarget = AcquiredTarget;
		// Debug::Print(FString::Printf(TEXT("[Leviosa] Target Acquired: %s"), *LevitatedTarget->GetName()), FColor::Cyan);
	}
	else
	{
		// Debug::Print(TEXT("[Leviosa] No valid Target. (Casting empty)"), FColor::Yellow);
	}

	// 2. 캐스팅 애니메이션 실행 (지팡이를 휘두르는 모션 등)
	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
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

				// TODO: 애니메이션의 알맞은 타이밍(AnimNotify)에서 StartLevitation() 호출하도록 변경할 수 있음.
				// 현재는 스크립트 실행 직후 바로 부유 상태 진입
				StartLevitation();
				return;
			}
		}
	}

	// 몽타주 재생에 실패했거나 세팅되어 있지 않다면 즉시 로직 처리
	StartLevitation();
}

void UGA_Spell_Leviosa::StartLevitation()
{
	// 향후 구현될 부분
	// 1. 대상(LevitatedTarget)의 물리 시뮬레이션 On/Off 및 위치(Transform) 강제 업데이트.
	// 2. AbilityTask_WaitInputPress / Release 등을 연결하여 F, V, Q, E 키 입력 시
	//    거리(Distance) 조절 오프셋 처리.
	// 또는, 간단히 위로 떠오르게만 하도록 처리

	if (IsValid(LevitatedTarget))
	{
		// Debug::Print(FString::Printf(TEXT("[Leviosa] Started Levitating %s! (Awaiting Input/Physics Logic)"), *LevitatedTarget->GetName()), FColor::Green);

		// 대상이 허공에 뜨는 상태를 부여하는 GameplayEffect나 Tag (ex: State.Levitated)를 해당 액터에게 적용하는 로직 추가 가능
	}
}

void UGA_Spell_Leviosa::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// 사용자가 마우스/버튼을 떼기 전까지 유지되는 '지속형(Sustained) 어빌리티'일 경우
	// 여기서 EndAbility를 호출하지 않고 대기
	// 하지만 현재는 기본 뼈대 점검을 위해 애니메이션 종료 시 안전하게 Ability를 종료

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
}

void UGA_Spell_Leviosa::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 부유 해제에 따른 정리 작업
	if (IsValid(LevitatedTarget))
	{
		// TODO: 대상의 중력을 복구하고 원래 물리 상태로 되돌리는 로직 등
		// Debug::Print(FString::Printf(TEXT("[Leviosa] Dropped %s"), *LevitatedTarget->GetName()), FColor::Red);

		LevitatedTarget = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}