#include "Component/SpellComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"

USpellComponent::USpellComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USpellComponent::BeginPlay()
{
	Super::BeginPlay();
}

void USpellComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (ActiveCooldownMap.Num() == 0)
	{
		return;
	}

	TArray<FGameplayTag> FinishedSpellIDs;

	for (TPair<FGameplayTag, float>& Pair : ActiveCooldownMap)
	{
		Pair.Value = FMath::Max(0.0f, Pair.Value - DeltaTime);

		if (Pair.Value <= KINDA_SMALL_NUMBER)
		{
			FinishedSpellIDs.Add(Pair.Key);
		}
	}

	for (const FGameplayTag& FinishedSpellID : FinishedSpellIDs)
	{
		ActiveCooldownMap.Remove(FinishedSpellID);
		TotalCooldownMap.Remove(FinishedSpellID);
		OnSpellCooldownEnded.Broadcast(FinishedSpellID);
	}
}

FSpellCastCheckResult USpellComponent::CanCastSpell(const FSpellCastRequest& CastRequest) const
{
	FSpellCastCheckResult Result;
	Result.bCanCast = false;
	Result.FailReason = ESpellCastFailReason::None;
	Result.BlockingTag = FGameplayTag();

	if (!CastRequest.SpellID.IsValid())
	{
		Result.FailReason = ESpellCastFailReason::InvalidSpellID;
		return Result;
	}

	if (!GetOwner())
	{
		Result.FailReason = ESpellCastFailReason::InvalidOwner;
		return Result;
	}

	if (bSpellCastingLocked)
	{
		Result.FailReason = ESpellCastFailReason::SpellCastingLocked;
		return Result;
	}

	FGameplayTag BlockingTag;
	if (FindBlockingOwnerTag(CastRequest, BlockingTag))
	{
		Result.FailReason = ESpellCastFailReason::BlockedByStateTag;
		Result.BlockingTag = BlockingTag;
		return Result;
	}

	if (!ShouldIgnoreCooldownCheck(CastRequest))
	{
		if (IsSpellOnCooldown(CastRequest.SpellID))
		{
			Result.FailReason = ESpellCastFailReason::SpellOnCooldown;
			return Result;
		}
	}

	Result.bCanCast = true;
	Result.FailReason = ESpellCastFailReason::None;
	return Result;
}

void USpellComponent::NotifySpellCastSuccess(const FSpellCastRequest& CastRequest)
{
	if (!CastRequest.SpellID.IsValid())
	{
		return;
	}

	OnSpellCastSucceeded.Broadcast(CastRequest.SpellID, CastRequest.CastContext);

	if (ShouldStartCooldownAfterCast(CastRequest))
	{
		if (CastRequest.CooldownSeconds > 0.0f)
		{
			StartCooldown(CastRequest.SpellID, CastRequest.CooldownSeconds);
		}
	}
}

void USpellComponent::NotifySpellCastFailed(
	FGameplayTag SpellID,
	ESpellCastFailReason FailReason
)
{
	OnSpellCastFailed.Broadcast(SpellID, FailReason);
}

void USpellComponent::StartCooldown(
	FGameplayTag SpellID,
	float CooldownSeconds
)
{
	if (!SpellID.IsValid())
	{
		return;
	}

	if (CooldownSeconds <= 0.0f)
	{
		ActiveCooldownMap.Remove(SpellID);
		TotalCooldownMap.Remove(SpellID);
		return;
	}

	ActiveCooldownMap.FindOrAdd(SpellID) = CooldownSeconds;
	TotalCooldownMap.FindOrAdd(SpellID) = CooldownSeconds;

	OnSpellCooldownStarted.Broadcast(SpellID, CooldownSeconds);
}

void USpellComponent::ClearCooldown(FGameplayTag SpellID)
{
	if (!SpellID.IsValid())
	{
		return;
	}

	const bool bRemovedActive = (ActiveCooldownMap.Remove(SpellID) > 0);
	TotalCooldownMap.Remove(SpellID);

	if (bRemovedActive)
	{
		OnSpellCooldownEnded.Broadcast(SpellID);
	}
}

bool USpellComponent::IsSpellOnCooldown(FGameplayTag SpellID) const
{
	if (!SpellID.IsValid())
	{
		return false;
	}

	const float* Found = ActiveCooldownMap.Find(SpellID);
	return (Found != nullptr && *Found > KINDA_SMALL_NUMBER);
}

bool USpellComponent::IsSpellReady(FGameplayTag SpellID) const
{
	return !IsSpellOnCooldown(SpellID);
}

float USpellComponent::GetRemainingCooldown(FGameplayTag SpellID) const
{
	if (!SpellID.IsValid())
	{
		return 0.0f;
	}

	const float* Found = ActiveCooldownMap.Find(SpellID);
	return Found ? *Found : 0.0f;
}

float USpellComponent::GetTotalCooldown(FGameplayTag SpellID) const
{
	if (!SpellID.IsValid())
	{
		return 0.0f;
	}

	const float* Found = TotalCooldownMap.Find(SpellID);
	return Found ? *Found : 0.0f;
}

void USpellComponent::SetSpellCastingLocked(bool bLocked)
{
	bSpellCastingLocked = bLocked;
}

void USpellComponent::GetActiveCooldownMap(TMap<FGameplayTag, float>& OutCooldownMap) const
{
	OutCooldownMap = ActiveCooldownMap;
}

void USpellComponent::GetActiveCooldownEntries(TArray<FSpellCooldownEntry>& OutEntries) const
{
	OutEntries.Reset();

	for (const TPair<FGameplayTag, float>& Pair : ActiveCooldownMap)
	{
		const float TotalTime = GetTotalCooldown(Pair.Key);
		OutEntries.Add(FSpellCooldownEntry(Pair.Key, Pair.Value, TotalTime));
	}
}

bool USpellComponent::ShouldIgnoreCooldownCheck(const FSpellCastRequest& CastRequest) const
{
	if (CastRequest.bForceIgnoreCooldownCheck)
	{
		return true;
	}

	switch (CastRequest.CastContext)
	{
	case ESpellCastContext::Normal:
		return false;

	case ESpellCastContext::ParryCounter:
		return true;

	case ESpellCastContext::SpecialFreeCast:
		return true;

	default:
		return false;
	}
}

bool USpellComponent::ShouldStartCooldownAfterCast(const FSpellCastRequest& CastRequest) const
{
	if (CastRequest.bForceStartCooldown)
	{
		return true;
	}

	switch (CastRequest.CastContext)
	{
	case ESpellCastContext::Normal:
		return true;

	case ESpellCastContext::ParryCounter:
		return false;

	case ESpellCastContext::SpecialFreeCast:
		return false;

	default:
		return true;
	}
}

bool USpellComponent::FindBlockingOwnerTag(
	const FSpellCastRequest& CastRequest,
	FGameplayTag& OutBlockingTag
) const
{
	OutBlockingTag = FGameplayTag();

	if (CastRequest.bIgnoreStateBlock)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		return false;
	}

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	for (const FGameplayTag& Tag : BlockingOwnerTags)
	{
		if (Tag.IsValid() && OwnedTags.HasTag(Tag))
		{
			OutBlockingTag = Tag;
			return true;
		}
	}

	return false;
}

UAbilitySystemComponent* USpellComponent::GetOwnerASC() const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OwnerActor))
	{
		return ASI->GetAbilitySystemComponent();
	}

	return OwnerActor->FindComponentByClass<UAbilitySystemComponent>();
}