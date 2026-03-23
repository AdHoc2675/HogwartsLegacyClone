#include "GAS/Abilities/Spell/Stupefy/GA_Spell_Stupefy.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

#include "Component/CombatComponent.h"

#include "Core/HOG_Struct.h"
#include "GameplayEffect.h"
#include "HOGDebugHelper.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

UGA_SpellStupefy::UGA_SpellStupefy()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

FSpellCastRequest UGA_SpellStupefy::BuildSpellCastRequest(ESpellCastContext CastContext) const
{
	FSpellCastRequest Request = Super::BuildSpellCastRequest(CastContext);

	// 패링 반격은 쿨타임 무시
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

	FGameplayTagContainer RelevantTags;
	if (!CheckCooldown(Handle, ActorInfo, &RelevantTags))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 프리 캐스트 페이싱
	if (TryBeginPreCastFacing(Handle, ActorInfo, ActivationInfo, TriggerEventData))
	{
		return;
	}

	ExecuteStupefyCast(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_SpellStupefy::OnPreCastFacingFinished(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	ExecuteStupefyCast(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_SpellStupefy::ExecuteStupefyCast(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
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

	// -------------------------
	// Notify용 데이터 저장
	// -------------------------
	PendingResolvedCastContext = CastContext;
	PendingResolvedTargetActor = TargetActor;
	PendingResolvedTargetTags = TargetTags;
	PendingResolvedAimPoint = AimPoint;
	bCastNotifyHandled = false;

	// -------------------------
	// Beam VFX 큐 등록
	// -------------------------
	QueueLineTraceSpellVFX(
		LineTraceBeamVFX,
		AimPoint,
		BeamStartSocketName,
		TEXT("BeamStart"),
		TEXT("BeamEnd"),
		TEXT("BeamLength")
	);

	// Notify가 이 Ability 호출 가능하도록 등록
	RegisterCastNotifyToOwner();

	// -------------------------
	// 몽타주 재생
	// -------------------------
	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character && Character->GetMesh() && CastMontage)
	{
		if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
		{
			const float Duration = AnimInstance->Montage_Play(CastMontage, 1.0f);
			if (Duration > 0.f)
			{
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &UGA_SpellStupefy::OnMontageEnded);
				AnimInstance->Montage_SetEndDelegate(EndDelegate, CastMontage);
				return;
			}
		}
	}

	// fallback (몽타주 없을 때)
	HandleCastNotify();
}

void UGA_SpellStupefy::HandleCastNotify()
{
	if (bCastNotifyHandled)
	{
		return;
	}

	bCastNotifyHandled = true;

	const bool bApplied = ApplyStupefyToTarget(
		PendingResolvedCastContext,
		PendingResolvedTargetActor,
		PendingResolvedTargetTags
	);

	if (!bApplied)
	{
		ResetPendingCastData();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	NotifySpellCastSucceeded(PendingResolvedCastContext);

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
	const ESpellCastContext Resolved = PendingCastContext;
	PendingCastContext = ESpellCastContext::Normal;
	return Resolved;
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

	if (InCastContext == ESpellCastContext::ParryCounter)
	{
		if (IsValid(PendingForcedTarget) && DoesTargetMeetRequirements(PendingForcedTarget))
		{
			OutTarget = PendingForcedTarget;
			OutAimPoint = PendingForcedTarget->GetActorLocation() + FVector(0,0,60);
			return true;
		}
	}

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

	UCombatComponent* CombatComp = TargetActor->FindComponentByClass<UCombatComponent>();
	if (!CombatComp)
	{
		return false;
	}

	FDamageRequest Request;
	Request.SourceActor = GetAvatarActorFromActorInfo();
	Request.TargetActor = TargetActor;
	Request.BaseDamage = GetBaseDamage();
	Request.TargetTags = TargetTags;

	FDamageResult Result = CombatComp->ApplyDamageRequest(Request);

	if (!Result.bWasApplied)
	{
		return false;
	}

	if (InCastContext == ESpellCastContext::ParryCounter)
	{
		ApplyStunEffectToTarget(TargetActor);
	}

	return true;
}

bool UGA_SpellStupefy::ApplyStunEffectToTarget(AActor* TargetActor)
{
	if (!IsValid(TargetActor) || !StunEffectClass)
	{
		return false;
	}

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor);
	if (!ASI)
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();

	if (!TargetASC || !SourceASC)
	{
		return false;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(StunEffectClass, GetAbilityLevel(), Context);

	if (!Spec.IsValid())
	{
		return false;
	}

	return SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC).WasSuccessfullyApplied();
}

void UGA_SpellStupefy::ResetPendingCastData()
{
	PendingCastContext = ESpellCastContext::Normal;
	PendingForcedTarget = nullptr;

	PendingResolvedCastContext = ESpellCastContext::Normal;
	PendingResolvedTargetActor = nullptr;
	PendingResolvedTargetTags.Reset();
	PendingResolvedAimPoint = FVector::ZeroVector;

	bCastNotifyHandled = false;
}

void UGA_SpellStupefy::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bCastNotifyHandled)
	{
		return;
	}

	if (bInterrupted)
	{
		ResetPendingCastData();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	HandleCastNotify();
}