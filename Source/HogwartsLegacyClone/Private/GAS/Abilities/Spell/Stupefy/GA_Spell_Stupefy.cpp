#include "GAS/Abilities/Spell/Stupefy/GA_Spell_Stupefy.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Component/CombatComponent.h"
#include "Core/HOG_Struct.h"
#include "GameplayEffect.h"
#include "HOGDebugHelper.h"
#include "Component/CombatComponent.h"

#include "GameFramework/Actor.h"

UGA_SpellStupefy::UGA_SpellStupefy()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
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

		// Debug::Print(
		// 	FString::Printf(
		// 		TEXT("[GA_SpellStupefy] Cast Failed | SpellID=%s | Reason=%d"),
		// 		*SpellID.ToString(),
		// 		static_cast<int32>(CheckResult.FailReason)
		// 	),
		// 	FColor::Red
		// );

		ResetPendingCastData();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	AActor* TargetActor = nullptr;
	FGameplayTagContainer TargetTags;
	FVector AimPoint = FVector::ZeroVector;

	if (!ResolveTargetForCast(CastContext, TargetActor, TargetTags, AimPoint))
	{
		// Debug::Print(TEXT("[GA_SpellStupefy] No valid target resolved."), FColor::Red);

		ResetPendingCastData();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		// Debug::Print(TEXT("[GA_SpellStupefy] CommitAbility failed."), FColor::Red);

		ResetPendingCastData();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 중요:
	// 슬로우 모션은 여기서 직접 처리하지 않음.
	// 패링 반격 연출은 애니메이션의 ANS_Stupefy_Slowmotion 에서 처리.

	const bool bApplied = ApplyStupefyToTarget(CastContext, TargetActor, TargetTags);
	if (!bApplied)
	{
		// Debug::Print(TEXT("[GA_SpellStupefy] ApplyStupefyToTarget failed."), FColor::Red);

		ResetPendingCastData();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	NotifySpellCastSucceeded(CastContext);

	// Debug::Print(
	// 	FString::Printf(
	// 		TEXT("[GA_SpellStupefy] Success | Context=%d | Target=%s | SlowMotionANS=%s"),
	// 		static_cast<int32>(CastContext),
	// 		*GetNameSafe(TargetActor),
	// 		*ParrySlowMotionANSName.ToString()
	// 	),
	// 	FColor::Cyan
	// );

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

	UCombatComponent* CombatComp = SourceActor->FindComponentByClass<UCombatComponent>();
	if (!CombatComp)
	{
		// Debug::Print(TEXT("[GA_SpellStupefy] CombatComponent missing on SourceActor."), FColor::Red);
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
		// Debug::Print(
		// 	FString::Printf(TEXT("[GA_SpellStupefy] Damage not applied | Target=%s"), *GetNameSafe(TargetActor)),
		// 	FColor::Red
		// );
		return false;
	}

	// 일반 시전은 데미지만
	if (InCastContext == ESpellCastContext::Normal)
	{
		return true;
	}

	// 패링 반격 / 특수 반격 계열은 기절 추가 가능
	if (InCastContext == ESpellCastContext::ParryCounter)
	{
		ApplyStunEffectToTarget(TargetActor);
	}

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
		// Debug::Print(TEXT("[GA_SpellStupefy] StunEffectClass is null. Stun skipped."), FColor::Yellow);
		return false;
	}

	if (!TargetActor->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
	{
		// Debug::Print(TEXT("[GA_SpellStupefy] Target has no ASC. Stun skipped."), FColor::Yellow);
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

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetAvatarActorFromActorInfo());
	if (!SourceASC)
	{
		// Debug::Print(TEXT("[GA_SpellStupefy] Source ASC missing. Stun skipped."), FColor::Yellow);
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(StunEffectClass, GetAbilityLevel(), EffectContext);
	if (!SpecHandle.IsValid())
	{
		// Debug::Print(TEXT("[GA_SpellStupefy] Failed to make stun effect spec."), FColor::Red);
		return false;
	}

	const FActiveGameplayEffectHandle ActiveGEHandle = SourceASC->ApplyGameplayEffectSpecToTarget(
		*SpecHandle.Data.Get(),
		TargetASC
	);

	const bool bApplied = ActiveGEHandle.WasSuccessfullyApplied();
	if (bApplied)
	{
		// Debug::Print(
		// 	FString::Printf(TEXT("[GA_SpellStupefy] Stun applied | Target=%s"), *GetNameSafe(TargetActor)),
		// 	FColor::Yellow
		// );
	}

	return bApplied;
}

void UGA_SpellStupefy::ResetPendingCastData()
{
	PendingCastContext = ESpellCastContext::Normal;
	PendingForcedTarget = nullptr;
}