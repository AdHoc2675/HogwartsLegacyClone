#include "GAS/Abilities/Enemy/GA_MeleeAttack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/BaseCharacter.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "Component/CombatComponent.h"
#include "Core/HOG_GameplayTags.h"
#include "Core/HOG_Struct.h"
#include "Data/Enemy/DA_EnemyConfigBase.h"
#include "Data/Enemy/DA_MeleeEnemyConfig.h"
#include "Data/Enemy/FEnemyAttackData.h"
#include "GameFramework/Controller.h"
#include "HOGDebugHelper.h"

UGA_MeleeAttack::UGA_MeleeAttack()
{
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
		// Debug::Print(TEXT("[GA_MeleeAttack] ActivateAbility failed | AttackMontage is null"), FColor::Red);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	// 몽타주 실행 이벤트
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		AttackMontage);

	if (!MontageTask)
	{
		// Debug::Print(TEXT("[GA_MeleeAttack] ActivateAbility failed | MontageTask is null"), FColor::Red);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

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
		this,
		HOGGameplayTags::Event_Weapon_Hit);

	if (!WaitEventTask)
	{
		// Debug::Print(TEXT("[GA_MeleeAttack] ActivateAbility failed | WaitEventTask is null"), FColor::Red);
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	WaitEventTask->EventReceived.AddDynamic(this, &UGA_MeleeAttack::OnWeaponHit);
	WaitEventTask->ReadyForActivation();

	// Debug::Print(TEXT("[GA_MeleeAttack] ActivateAbility success | Montage + WaitGameplayEvent started"), FColor::Green);
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
	// Debug::Print(TEXT("[GA_MeleeAttack] Montage completed"), FColor::Green);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_MeleeAttack::OnMontageCancelled()
{
	// 피격 등, 몽타주가 중간에 실행을 멈추는 경우
	// Debug::Print(TEXT("[GA_MeleeAttack] Montage cancelled/interrupted"), FColor::Yellow);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_MeleeAttack::OnWeaponHit(FGameplayEventData Payload)
{
	//Debug::Print(TEXT("[GA_MeleeAttack] OnWeaponHit called"), FColor::Orange);

	// 맞은 대상 꺼냄(플레이어)
	AActor* TargetActor = const_cast<AActor*>(Payload.Target.Get());
	if (!TargetActor)
	{
		// Debug::Print(TEXT("[GA_MeleeAttack] OnWeaponHit failed | TargetActor is null"), FColor::Red);
		return;
	}

	AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(GetCharacter());
	if (!Enemy)
	{
		// Debug::Print(TEXT("[GA_MeleeAttack] OnWeaponHit failed | Enemy is null"), FColor::Red);
		return;
	}

	ABaseCharacter* TargetCharacter = Cast<ABaseCharacter>(TargetActor);
	if (!TargetCharacter)
	{
		// Debug::Print(FString::Printf(
		// 	TEXT("[GA_MeleeAttack] OnWeaponHit failed | Target is not BaseCharacter | Target=%s"),
		// 	*GetNameSafe(TargetActor)
		// ), FColor::Red);
		return;
	}

	UCombatComponent* TargetCombatComponent = TargetCharacter->GetCombatComponent();
	if (!TargetCombatComponent)
	{
		// Debug::Print(FString::Printf(
		// 	TEXT("[GA_MeleeAttack] OnWeaponHit failed | TargetCombatComponent is null | Target=%s"),
		// 	*GetNameSafe(TargetCharacter)
		// ), FColor::Red);
		return;
	}

	float Damage = GetMeleeAttackDamage();
	if (Damage <= 0.f)
	{
		// Debug::Print(FString::Printf(
		// 	TEXT("[GA_MeleeAttack] OnWeaponHit failed | Damage <= 0 | Damage=%.2f"),
		// 	Damage
		// ), FColor::Red);
		return;
	}

	FDamageRequest DamageRequest;
	DamageRequest.SourceActor = Enemy;
	DamageRequest.TargetActor = TargetCharacter;
	DamageRequest.InstigatorActor = Enemy;
	DamageRequest.DamageCauser = Enemy;
	DamageRequest.BaseDamage = Damage;
	DamageRequest.HitResult = Payload.ContextHandle.GetHitResult() ? *Payload.ContextHandle.GetHitResult() : FHitResult();

	// 필요 시 후속 확장
	// DamageRequest.DamageTypeTag = HOGGameplayTags::DamageType_Melee;

	// Debug::Print(FString::Printf(
	// 	TEXT("[GA_MeleeAttack] ApplyDamageRequest | Source=%s | Target=%s | BaseDamage=%.2f"),
	// 	*GetNameSafe(DamageRequest.SourceActor),
	// 	*GetNameSafe(DamageRequest.TargetActor),
	// 	DamageRequest.BaseDamage
	// ), FColor::Cyan);

	const FDamageResult DamageResult = TargetCombatComponent->ApplyDamageRequest(DamageRequest);

	// Debug::Print(FString::Printf(
	// 	TEXT("[GA_MeleeAttack] DamageResult | Applied=%d | Blocked=%d | Parried=%d | Killed=%d | FinalDamage=%.2f"),
	// 	DamageResult.bWasApplied ? 1 : 0,
	// 	DamageResult.bWasBlocked ? 1 : 0,
	// 	DamageResult.bWasParried ? 1 : 0,
	// 	DamageResult.bKilledTarget ? 1 : 0,
	// 	DamageResult.FinalDamage
	// ), FColor::Green);
}