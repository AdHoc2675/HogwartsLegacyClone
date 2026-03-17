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
		Request.bForceStartCooldown = true;
	}

	return Request;
}

void UGA_SpellStupefy::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	Debug::Print(
		FString::Printf(
			TEXT("[GA_Spell_Stupefy] ActivateAbility | Avatar=%s | SpellID=%s"),
			*GetNameSafe(GetAvatarActorFromActorInfo()),
			*SpellID.ToString()
		)
	);

	const ESpellCastContext CastContext = ResolveCastContextAndConsume();
	const FSpellCastRequest CastRequest = BuildSpellCastRequest(CastContext);

	Debug::Print(
		FString::Printf(
			TEXT("[GA_Spell_Stupefy] CastContext=%d | ForcedTarget=%s | IgnoreCooldown=%d | ForceStartCooldown=%d"),
			static_cast<int32>(CastContext),
			*GetNameSafe(PendingForcedTarget),
			CastRequest.bForceIgnoreCooldownCheck ? 1 : 0,
			CastRequest.bForceStartCooldown ? 1 : 0
		)
	);

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
		Debug::Print(
			FString::Printf(
				TEXT("[GA_Spell_Stupefy] Cast Failed | FailReason=%d"),
				static_cast<int32>(CheckResult.FailReason)
			)
		);

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
		Debug::Print(TEXT("[GA_Spell_Stupefy] ResolveTargetForCast Failed"));

		ResetPendingCastData();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	Debug::Print(
		FString::Printf(
			TEXT("[GA_Spell_Stupefy] TargetResolved | Target=%s | AimPoint=%s"),
			*GetNameSafe(TargetActor),
			*AimPoint.ToString()
		)
	);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		Debug::Print(TEXT("[GA_Spell_Stupefy] CommitAbility Failed"));

		ResetPendingCastData();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 슬로우 모션은 Ability에서 직접 처리하지 않음.
	// 패링 반격 연출은 애니메이션의 ANS_Stupefy_Slowmotion 에서 처리.
	const bool bApplied = ApplyStupefyToTarget(CastContext, TargetActor, TargetTags);
	if (!bApplied)
	{
		Debug::Print(TEXT("[GA_Spell_Stupefy] ApplyStupefyToTarget Failed"));

		ResetPendingCastData();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	Debug::Print(
		FString::Printf(
			TEXT("[GA_Spell_Stupefy] Success | Context=%d | Target=%s"),
			static_cast<int32>(CastContext),
			*GetNameSafe(TargetActor)
		)
	);

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

			Debug::Print(
				FString::Printf(
					TEXT("[GA_Spell_Stupefy] ResolveTargetForCast | Parry ForcedTarget Used=%s"),
					*GetNameSafe(OutTarget)
				)
			);

			return true;
		}

		Debug::Print(TEXT("[GA_Spell_Stupefy] ResolveTargetForCast | Parry ForcedTarget Invalid -> Fallback LockOn"));
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
		Debug::Print(TEXT("[GA_Spell_Stupefy] ApplyStupefyToTarget | TargetActor Invalid"));
		return false;
	}

	AActor* SourceActor = GetAvatarActorFromActorInfo();
	if (!IsValid(SourceActor))
	{
		Debug::Print(TEXT("[GA_Spell_Stupefy] ApplyStupefyToTarget | SourceActor Invalid"));
		return false;
	}

	Debug::Print(
		FString::Printf(
			TEXT("[GA_Spell_Stupefy] ApplyStupefyToTarget | Context=%d | Target=%s | BaseDamage=%.2f"),
			static_cast<int32>(InCastContext),
			*GetNameSafe(TargetActor),
			GetBaseDamage()
		)
	);

	UCombatComponent* CombatComp = TargetActor->FindComponentByClass<UCombatComponent>();
	if (!CombatComp)
	{
		Debug::Print(TEXT("[GA_Spell_Stupefy] ApplyStupefyToTarget | CombatComponent Missing"));
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

	Debug::Print(
		FString::Printf(
			TEXT("[GA_Spell_Stupefy] DamageApplied=%d | Target=%s"),
			DamageResult.bWasApplied ? 1 : 0,
			*GetNameSafe(TargetActor)
		)
	);

	if (!DamageResult.bWasApplied)
	{
		return false;
	}

	const bool bStunApplied = ApplyStunEffectToTarget(TargetActor);

	Debug::Print(
		FString::Printf(
			TEXT("[GA_Spell_Stupefy] StunApplied=%d | Target=%s"),
			bStunApplied ? 1 : 0,
			*GetNameSafe(TargetActor)
		)
	);

	// 데미지가 이미 적용됐다면 스킬 자체는 성공으로 본다.
	return true;
}

bool UGA_SpellStupefy::ApplyStunEffectToTarget(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		Debug::Print(TEXT("[GA_Spell_Stupefy] ApplyStunEffectToTarget | Target Invalid"));
		return false;
	}

	if (!StunEffectClass)
	{
		Debug::Print(TEXT("[GA_Spell_Stupefy] ApplyStunEffectToTarget | StunEffectClass Null"));
		return false;
	}

	if (!TargetActor->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
	{
		Debug::Print(TEXT("[GA_Spell_Stupefy] ApplyStunEffectToTarget | Target Has No AbilitySystemInterface"));
		return false;
	}

	IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(TargetActor);
	if (!TargetASI)
	{
		Debug::Print(TEXT("[GA_Spell_Stupefy] ApplyStunEffectToTarget | TargetASI Cast Failed"));
		return false;
	}

	UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
	if (!TargetASC)
	{
		Debug::Print(TEXT("[GA_Spell_Stupefy] ApplyStunEffectToTarget | TargetASC Null"));
		return false;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		Debug::Print(TEXT("[GA_Spell_Stupefy] ApplyStunEffectToTarget | SourceASC Null"));
		return false;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(StunEffectClass, GetAbilityLevel(), EffectContext);
	if (!SpecHandle.IsValid())
	{
		Debug::Print(TEXT("[GA_Spell_Stupefy] ApplyStunEffectToTarget | SpecHandle Invalid"));
		return false;
	}

	const FActiveGameplayEffectHandle ActiveGEHandle = SourceASC->ApplyGameplayEffectSpecToTarget(
		*SpecHandle.Data.Get(),
		TargetASC
	);

	const bool bApplied = ActiveGEHandle.WasSuccessfullyApplied();

	Debug::Print(
		FString::Printf(
			TEXT("[GA_Spell_Stupefy] ApplyStunEffectToTarget | Applied=%d | Target=%s"),
			bApplied ? 1 : 0,
			*GetNameSafe(TargetActor)
		)
	);

	return bApplied;
}

void UGA_SpellStupefy::ResetPendingCastData()
{
	PendingCastContext = ESpellCastContext::Normal;
	PendingForcedTarget = nullptr;
}