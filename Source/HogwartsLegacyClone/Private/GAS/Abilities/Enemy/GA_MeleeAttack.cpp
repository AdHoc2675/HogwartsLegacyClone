// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/Enemy/GA_MeleeAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Core/HOG_GameplayTags.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "Data/Enemy/DA_EnemyConfigBase.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Data/Enemy/DA_MeleeEnemyConfig.h"
#include "Data/Enemy/FEnemyAttackData.h"

UGA_MeleeAttack::UGA_MeleeAttack()
{
	// 어빌리티 태그 등록
	AbilityTags.AddTag(HOGGameplayTags::Ability_Enemy_MeleeAttack);

	// 어빌리티 시작되면 자동 추가, 끝나면 자동 제거, 공격중인 상태에서 새로운 공격 block
	ActivationOwnedTags.AddTag(HOGGameplayTags::State_Attacking);

	// 공격 중 일 때, Block, ActivationOwnedTags.AddTag로 태그를 동적으로 추가하면 block
	ActivationBlockedTags.AddTag(HOGGameplayTags::State_Attacking);

	// 피격 중 일 때, Block, ActivationOwnedTags.AddTag로 태그를 동적으로 추가하면 block
	ActivationBlockedTags.AddTag(HOGGameplayTags::State_Hit);
}

void UGA_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	UAnimMontage* AttackMontage = GetAttackMontage();
	if (!AttackMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 몽타주 실행 이벤트
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		AttackMontage);

	// 몽타주가 끝날 때
	MontageTask->OnCompleted.AddDynamic(this, &UGA_MeleeAttack::OnMontageCompleted);

	// 몽타주 예외 상황
	// ex) 피격 etc
	MontageTask->OnCancelled.AddDynamic(this, &UGA_MeleeAttack::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_MeleeAttack::OnMontageCancelled);

	// 몽타주 Task 실행
	MontageTask->ReadyForActivation();

	// 히트 이벤트 대기
	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, HOGGameplayTags::Event_Weapon_Hit);
	WaitEventTask->EventReceived.AddDynamic(this, &UGA_MeleeAttack::OnWeaponHit);
	WaitEventTask->ReadyForActivation();
}


float UGA_MeleeAttack::GetMeleeAttackDamage() const
{
	const FEnemyAttackData* Data = GetAttackData();
	return Data ? Data->Damage : 1.f;
}

UAnimMontage* UGA_MeleeAttack::GetAttackMontage() const
{
	const FEnemyAttackData* Data = GetAttackData();
	return Data ? Data->AnimMontage : nullptr;
	
}

const FEnemyAttackData* UGA_MeleeAttack::GetAttackData() const
{
	AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(GetCharacter());
	if (!Enemy) return nullptr;
	
	UDA_MeleeEnemyConfig* Config = Cast<UDA_MeleeEnemyConfig>(Enemy->GetEnemyConfig());
	if (!Config) return nullptr;
	
	FGameplayTag Tag = AbilityTags.First();
	return Tag.IsValid() ? Config->FindAttackData(Tag) : nullptr;
	
}

void UGA_MeleeAttack::OnMontageCompleted()
{
	// 정상적으로 몽타주가 끝나는 경우
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_MeleeAttack::OnMontageCancelled()
{
	// 피격 등, 몽타주가 중간에 실행을 멈추는 경우
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_MeleeAttack::OnWeaponHit(FGameplayEventData Payload)
{
	UE_LOG(LogTemp, Warning, TEXT("Hit"));

	// 맞은 대상 꺼냄(플레이어)
	AActor* Target = const_cast<AActor*>(Payload.Target.Get());
	if (!Target)
	{
		UE_LOG(LogTemp, Warning, TEXT("Target"));
		return;
	}

	// UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	// if (!TargetASC)
	// {
	// 	UE_LOG(LogTemp, Warning, TEXT("no ASC"));
	// 	return;
	// }

	AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(GetCharacter());
	if (!Enemy)
	{
		UE_LOG(LogTemp, Warning, TEXT("no Enemy"));
		return;
	}

	UDA_EnemyConfigBase* Config = Enemy->GetEnemyConfig();
	if (!Config || !Config->DamageEffect)
	{
		UE_LOG(LogTemp, Warning, TEXT("no Config"));
		return;
	}

	float Damage = GetMeleeAttackDamage();
	if (Damage <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("no Damage"));
		return;
	}

	// 로그용
	// UE_LOG(LogTemp, Warning, TEXT("Hit Target: %s | Damage: %.1f"), 
	// *Target->GetName(), Damage);

	// // GE_Damage 명령서 생성
	// FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(Config->DamageEffect);
	//
	// // 데미지 주입
	// SpecHandle.Data->SetSetByCallerMagnitude(HOGGameplayTags::Damage_Melee, Damage);
	//
	// TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}
