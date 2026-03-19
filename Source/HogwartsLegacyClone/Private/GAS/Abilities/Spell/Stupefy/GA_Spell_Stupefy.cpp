#include "GAS/Abilities/Spell/Stupefy/GA_Spell_Stupefy.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Component/CombatComponent.h"
#include "Core/HOG_Struct.h"
#include "GameplayEffect.h"
#include "HOGDebugHelper.h"

#include "GameFramework/Actor.h"

UGA_SpellStupefy::UGA_SpellStupefy()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

FSpellCastRequest UGA_SpellStupefy::BuildSpellCastRequest(ESpellCastContext CastContext) const
{
	FSpellCastRequest Request = Super::BuildSpellCastRequest(CastContext);

	// 패링 반격 Stupefy만 쿨타임 체크 무시 + 발동 후 쿨타임 시작
	if (CastContext == ESpellCastContext::ParryCounter)
	{
		Request.bForceIgnoreCooldownCheck = true;
		Request.bForceStartCooldown = false;
	}

	return Request;
}

bool UGA_SpellStupefy::CheckCooldown(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags
) const
{
	if (PendingCastContext == ESpellCastContext::ParryCounter)
	{
		return true;
	}
	
	return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);
}

void UGA_SpellStupefy::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const ESpellCastContext CastContext = ResolveCastContextAndConsume();
	AActor* ForcedTargetSnapshot = PendingForcedTarget;
	const FSpellCastRequest CastRequest = BuildSpellCastRequest(CastContext);

	FSpellCastCheckResult CheckResult;
	switch (CastContext)
	{
	case ESpellCastContext::ParryCounter:
		CanCastAsParryCounter(CheckResult);
		break;

	case ESpellCastContext::SpecialFreeCast:
		CanCastAsSpecialFreeCast(CheckResult);
		break;

	case ESpellCastContext::Normal:
	default:
		CanCastAsNormal(CheckResult);
		break;
	}

	if (!CheckResult.bCanCast)
	{
		NotifySpellCastFailedResult(CastRequest, CheckResult);
		ResetPendingCastData();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	AActor* TargetActor = nullptr;
	FGameplayTagContainer TargetTags;
	FVector AimPoint = FVector::ZeroVector;

	if (!ResolveTargetForCast(CastContext, TargetActor, TargetTags, AimPoint))
	{
		ResetPendingCastData();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		ResetPendingCastData();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 슬로우 모션은 Ability에서 직접 처리하지 않음.
	// 패링 반격 연출은 애니메이션의 ANS_Stupefy_Slowmotion 에서 처리.
	const bool bApplied = ApplyStupefyToTarget(CastContext, TargetActor, TargetTags);
	if (!bApplied)
	{
		ResetPendingCastData();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	NotifySpellCastSucceeded(CastContext);

	ResetPendingCastData();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void UGA_SpellStupefy::PrepareCastContext(
	ESpellCastContext InCastContext,
	AActor* InForcedTarget
)
{
	PendingCastContext = InCastContext;
	PendingForcedTarget = InForcedTarget;
}

ESpellCastContext UGA_SpellStupefy::ResolveCastContextAndConsume()
{
	const ESpellCastContext ResolvedContext = PendingCastContext;
	return PendingCastContext;
}

bool UGA_SpellStupefy::ResolveTargetForCast(
	ESpellCastContext InCastContext,
	AActor*& OutTarget,
	FGameplayTagContainer& OutTargetTags,
	FVector& OutAimPoint
)
{
	OutTarget = nullptr;
	OutTargetTags.Reset();
	OutAimPoint = FVector::ZeroVector;

	// 패링 반격: 강제 타겟 우선
	if (InCastContext == ESpellCastContext::ParryCounter)
	{
		if (IsValid(PendingForcedTarget) && DoesTargetMeetRequirements(PendingForcedTarget))
		{
			OutTarget = PendingForcedTarget;
			OutAimPoint = PendingForcedTarget->GetActorLocation();

			if (PendingForcedTarget->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
			{
				if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PendingForcedTarget))
				{
					if (UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent())
					{
						TargetASC->GetOwnedGameplayTags(OutTargetTags);
					}
				}
			}

			return true;
		}
	}

	// 일반 시전 / fallback
	return TryConsumeLockedTarget(OutTarget, OutTargetTags, OutAimPoint);
}

bool UGA_SpellStupefy::ApplyStupefyToTarget(
	ESpellCastContext InCastContext,
	AActor* TargetActor,
	const FGameplayTagContainer& TargetTags
)
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	AActor* SourceActor = GetAvatarActorFromActorInfo();
	if (!IsValid(SourceActor))
	{
		return false;
	}

	UCombatComponent* CombatComp = TargetActor->FindComponentByClass<UCombatComponent>();
	if (!CombatComp)
	{
		return false;
	}

	FDamageRequest DamageRequest;
	DamageRequest.SourceActor = SourceActor;
	DamageRequest.TargetActor = TargetActor;
	DamageRequest.InstigatorActor = SourceActor;
	DamageRequest.DamageCauser = SourceActor;
	DamageRequest.BaseDamage = GetBaseDamage();
	DamageRequest.SourceTags = FGameplayTagContainer();
	DamageRequest.TargetTags = TargetTags;

	FDamageResult DamageResult = CombatComp->ApplyDamageRequest(DamageRequest);

	if (!DamageResult.bWasApplied)
	{
		return false;
	}

	const bool bStunApplied = ApplyStunEffectToTarget(TargetActor);

	// 데미지가 이미 적용됐다면 스킬 자체는 성공으로 본다.
	return true;
}

bool UGA_SpellStupefy::ApplyStunEffectToTarget(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return false;
	}

	if (!StunEffectClass)
	{
		return false;
	}

	if (!TargetActor->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
	{
		return false;
	}

	IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(TargetActor);
	if (!TargetASI)
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
	if (!TargetASC)
	{
		return false;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(StunEffectClass, GetAbilityLevel(), EffectContext);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	const FActiveGameplayEffectHandle ActiveGEHandle = SourceASC->ApplyGameplayEffectSpecToTarget(
		*SpecHandle.Data.Get(),
		TargetASC
	);

	const bool bApplied = ActiveGEHandle.WasSuccessfullyApplied();

	return bApplied;
}

void UGA_SpellStupefy::ResetPendingCastData()
{
	PendingCastContext = ESpellCastContext::Normal;
	PendingForcedTarget = nullptr;
}