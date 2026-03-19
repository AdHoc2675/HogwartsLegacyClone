#include "GAS/Abilities/GA_SpellBase.h"

#include "HOGDebugHelper.h"
#include "Data/DA_SpellDefinition.h"
#include "GameFramework/HOG_GameInstance.h"
#include "GameFramework/HOG_PlayerState.h"
#include "Component/SpellComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Character/Player/PlayerCharacterBase.h"
#include "Component/LockOnComponent.h"

UGA_SpellBase::UGA_SpellBase()
{
	// 기본적으로 SpellBase 자체는 “설정(Definition 조회/검증)”만 담당한다.
	// 실제 공격/퍼즐 로직은 파생 Ability에서 구현.
	// 따라서 별도의 정책 변경은 하지 않고 GA_Base의 기본 정책을 사용한다.

	bWarnIfDefinitionMissing = true;
}

UDA_SpellDefinition* UGA_SpellBase::GetSpellDefinition() const
{
	// 1) SpellID 자체가 유효하지 않으면 조회 불가.
	if (!SpellID.IsValid())
	{
		return nullptr;
	}

	// 2) GAS Ability는 런타임에 CurrentActorInfo가 세팅된다.
	if (!CurrentActorInfo)
	{
		return nullptr;
	}

	// 3) World -> GameInstance
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UHOG_GameInstance* GI = Cast<UHOG_GameInstance>(World->GetGameInstance());
	if (!GI)
	{
		return nullptr;
	}

	// 4) SpellRegistry에서 SpellID로 Definition 조회
	return GI->GetSpellDefinition(SpellID);
}

UDA_SpellDefinition* UGA_SpellBase::GetSpellDefinitionOrWarn() const
{
	UDA_SpellDefinition* Def = GetSpellDefinition();
	if (Def)
	{
		return Def;
	}

	if (!bWarnIfDefinitionMissing)
	{
		return nullptr;
	}

	const FString Msg = FString::Printf(
		TEXT("[GA_SpellBase] SpellDefinition missing. Ability=%s SpellID=%s"),
		*GetNameSafe(this),
		*SpellID.ToString()
	);

	return nullptr;
}

float UGA_SpellBase::GetCooldownSeconds() const
{
	if (UDA_SpellDefinition* Def = GetSpellDefinitionOrWarn())
	{
		return Def->CooldownSeconds;
	}
	return 0.f;
}

float UGA_SpellBase::GetBaseDamage() const
{
	if (UDA_SpellDefinition* Def = GetSpellDefinitionOrWarn())
	{
		return Def->BaseDamage;
	}
	return 0.f;
}

float UGA_SpellBase::GetCastRange() const
{
	if (UDA_SpellDefinition* Def = GetSpellDefinitionOrWarn())
	{
		return Def->CastRange;
	}
	return 0.f;
}

bool UGA_SpellBase::DoesTargetMeetRequirements(AActor* Target) const
{
	if (!IsValid(Target))
	{
		return false;
	}

	UDA_SpellDefinition* Def = GetSpellDefinitionOrWarn();
	if (!Def)
	{
		return false;
	}

	if (IsTargetBlocked(Target, Def->TargetBlockedTags))
	{
		return false;
	}

	if (!HasAllRequiredTags(Target, Def->TargetRequiredTags))
	{
		return false;
	}

	return true;
}

bool UGA_SpellBase::TryConsumeLockedTarget(
	AActor*& OutTarget,
	FGameplayTagContainer& OutTargetTags,
	FVector& OutAimPoint
) const
{
	OutTarget = nullptr;
	OutTargetTags.Reset();
	OutAimPoint = FVector::ZeroVector;

	UDA_SpellDefinition* Def = GetSpellDefinitionOrWarn();
	if (!Def)
	{
		BuildFallbackAimPoint(OutAimPoint, 2000.f);
		return false;
	}

	if (!CurrentActorInfo)
	{
		BuildFallbackAimPoint(OutAimPoint, Def->CastRange);
		return false;
	}

	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(Avatar);
	if (!PlayerCharacter)
	{
		BuildFallbackAimPoint(OutAimPoint, Def->CastRange);
		return false;
	}

	ULockOnComponent* LockOn = PlayerCharacter->GetLockOnComponent();
	if (!LockOn)
	{
		BuildFallbackAimPoint(OutAimPoint, Def->CastRange);
		return false;
	}

	FLockOnTargetResult LockedResult;
	const bool bHasLockedTarget = LockOn->TryGetLockedTargetResult(LockedResult);

	if (!bHasLockedTarget || !IsValid(LockedResult.TargetActor))
	{
		BuildFallbackAimPoint(OutAimPoint, Def->CastRange);
		return false;
	}

	if (!DoesTargetMeetRequirements(LockedResult.TargetActor))
	{
		OutAimPoint = LockedResult.AimPoint.IsNearlyZero()
			? LockedResult.TargetActor->GetActorLocation()
			: LockedResult.AimPoint;
		return false;
	}

	OutTarget = LockedResult.TargetActor;
	OutTargetTags = LockedResult.TargetTags;
	OutAimPoint = LockedResult.AimPoint.IsNearlyZero()
		? LockedResult.TargetActor->GetActorLocation()
		: LockedResult.AimPoint;

	return true;
}

bool UGA_SpellBase::BuildFallbackAimPoint(FVector& OutAimPoint, float RangeOverride) const
{
	if (!CurrentActorInfo)
	{
		return false;
	}

	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
	if (!Avatar)
	{
		return false;
	}

	APawn* Pawn = Cast<APawn>(Avatar);
	if (!Pawn)
	{
		return false;
	}

	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}

	const float UseRange = (RangeOverride > 0.f) ? RangeOverride : GetCastRange();
	const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
	const FVector CamForward = PC->PlayerCameraManager->GetActorForwardVector().GetSafeNormal();

	OutAimPoint = CamLoc + (CamForward * UseRange);
	return true;
}

void UGA_SpellBase::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (ShouldApplyCastingActiveTag())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->AddLooseGameplayTag(
				FGameplayTag::RequestGameplayTag(TEXT("State.Casting.Active"))
			);
		}
	}
}

void UGA_SpellBase::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled
)
{
	if (ShouldApplyCastingActiveTag())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveLooseGameplayTag(
				FGameplayTag::RequestGameplayTag(TEXT("State.Casting.Active"))
			);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

USpellComponent* UGA_SpellBase::GetSpellComponent() const
{
	if (!CurrentActorInfo)
	{
		return nullptr;
	}

	AActor* OwnerActor = CurrentActorInfo->OwnerActor.Get();
	if (!OwnerActor)
	{
		return nullptr;
	}

	AHOG_PlayerState* HOGPS = Cast<AHOG_PlayerState>(OwnerActor);
	if (!HOGPS)
	{
		return nullptr;
	}

	return HOGPS->GetSpellComponent();
}

FSpellCastRequest UGA_SpellBase::BuildSpellCastRequest(ESpellCastContext CastContext) const
{
	FSpellCastRequest Request;
	Request.SpellID = SpellID;
	Request.CastContext = CastContext;
	Request.CooldownSeconds = GetCooldownSeconds();

	// 기본 정책:
	// - Normal: 기본값 유지
	// - ParryCounter / SpecialFreeCast: SpellComponent가 컨텍스트로 처리
	Request.bIgnoreStateBlock = false;
	Request.bForceStartCooldown = false;
	Request.bForceIgnoreCooldownCheck = false;

	return Request;
}

FSpellCastCheckResult UGA_SpellBase::CheckCanCastSpell(ESpellCastContext CastContext) const
{
	FSpellCastCheckResult Result;
	Result.bCanCast = false;
	Result.FailReason = ESpellCastFailReason::InvalidOwner;

	USpellComponent* SpellComp = GetSpellComponent();
	if (!SpellComp)
	{
		return Result;
	}

	const FSpellCastRequest Request = BuildSpellCastRequest(CastContext);
	return SpellComp->CanCastSpell(Request);
}

void UGA_SpellBase::NotifySpellCastFailedResult(
	const FSpellCastRequest& CastRequest,
	const FSpellCastCheckResult& CheckResult
) const
{
	USpellComponent* SpellComp = GetSpellComponent();
	if (!SpellComp)
	{
		return;
	}

	SpellComp->NotifySpellCastFailed(CastRequest.SpellID, CheckResult.FailReason);
}

void UGA_SpellBase::NotifySpellCastSucceeded(ESpellCastContext CastContext) const
{
	USpellComponent* SpellComp = GetSpellComponent();
	if (!SpellComp)
	{
		return;
	}

	const FSpellCastRequest Request = BuildSpellCastRequest(CastContext);
	SpellComp->NotifySpellCastSuccess(Request);
}

bool UGA_SpellBase::CanCastAsNormal(FSpellCastCheckResult& OutCheckResult) const
{
	OutCheckResult = CheckCanCastSpell(ESpellCastContext::Normal);
	return OutCheckResult.bCanCast;
}

bool UGA_SpellBase::CanCastAsParryCounter(FSpellCastCheckResult& OutCheckResult) const
{
	OutCheckResult = CheckCanCastSpell(ESpellCastContext::ParryCounter);
	return OutCheckResult.bCanCast;
}

bool UGA_SpellBase::CanCastAsSpecialFreeCast(FSpellCastCheckResult& OutCheckResult) const
{
	OutCheckResult = CheckCanCastSpell(ESpellCastContext::SpecialFreeCast);
	return OutCheckResult.bCanCast;
}

bool UGA_SpellBase::IsTargetBlocked(AActor* Target, const FGameplayTagContainer& Blocked) const
{
	if (Blocked.IsEmpty())
	{
		return false;
	}

	if (Target->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
	{
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Target);
		if (ASI)
		{
			if (UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent())
			{
				FGameplayTagContainer Owned;
				TargetASC->GetOwnedGameplayTags(Owned);
				return Owned.HasAny(Blocked);
			}
		}
	}

	return false;
}

bool UGA_SpellBase::HasAllRequiredTags(AActor* Target, const FGameplayTagContainer& Required) const
{
	if (Required.IsEmpty())
	{
		return true;
	}

	if (Target->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
	{
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Target);
		if (ASI)
		{
			if (UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent())
			{
				FGameplayTagContainer Owned;
				TargetASC->GetOwnedGameplayTags(Owned);
				return Owned.HasAll(Required);
			}
		}
	}

	return false;
}

bool UGA_SpellBase::ShouldApplyCastingActiveTag() const
{
	return true;
}