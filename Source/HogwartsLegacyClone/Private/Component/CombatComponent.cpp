// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/CombatComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GAS/Attributes/HOGAttributeSet.h"
#include "Character/BaseCharacter.h"
#include "HOGDebugHelper.h"
#include "Core/HOG_GameplayTags.h"
#include "GameplayEffect.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffectExtension.h"



UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeCombatComponent();
}

void UCombatComponent::InitializeCombatComponent()
{
	AActor* MyOwner = GetOwner();
	if (!MyOwner)
	{
		DebugPrint(TEXT("[CombatComponent] Initialize failed | Owner is null"));
		return;
	}

	OwnerCharacter = Cast<ABaseCharacter>(MyOwner);
	if (!OwnerCharacter.IsValid())
	{
		DebugPrint(FString::Printf(
			TEXT("[CombatComponent] Initialize failed | Owner is not ABaseCharacter | Owner=%s"),
			*GetNameSafe(MyOwner)
		));
		return;
	}

	UAbilitySystemComponent* FoundASC = nullptr;

	// 1) Owner에서 먼저 시도
	FoundASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MyOwner);

	// 2) PlayerState에서 재시도
	if (!FoundASC)
	{
		if (const APawn* OwnerPawn = Cast<APawn>(MyOwner))
		{
			APlayerState* PS = OwnerPawn->GetPlayerState();
			if (PS)
			{
				FoundASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS);
			}
		}
	}

	AbilitySystemComponent = FoundASC;

	DebugPrint(FString::Printf(
		TEXT("[CombatComponent] Initialize | Owner=%s | ASC=%s"),
		*GetNameSafe(OwnerCharacter.Get()),
		*GetNameSafe(AbilitySystemComponent.Get())
	));
}

UAbilitySystemComponent* UCombatComponent::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent.Get();
}

const UHOGAttributeSet* UCombatComponent::GetAttributeSet() const
{
	if (!AbilitySystemComponent.IsValid())
	{
		return nullptr;
	}

	return AbilitySystemComponent->GetSet<UHOGAttributeSet>();
}

bool UCombatComponent::CanReceiveDamage() const
{
	if (!bCanBeDamaged)
	{
		return false;
	}

	if (bIsDead)
	{
		return false;
	}

	if (!OwnerCharacter.IsValid())
	{
		return false;
	}

	if (OwnerCharacter->IsDead())
	{
		return false;
	}

	if (!AbilitySystemComponent.IsValid())
	{
		return false;
	}

	return true;
}

bool UCombatComponent::IsDead() const
{
	if (bIsDead)
	{
		return true;
	}

	if (OwnerCharacter.IsValid())
	{
		return OwnerCharacter->IsDead();
	}

	return false;
}

bool UCombatComponent::IsFriendlyTo(AActor* OtherActor) const
{
	if (!OwnerCharacter.IsValid() || !OtherActor)
	{
		return false;
	}

	const ABaseCharacter* OtherCharacter = Cast<ABaseCharacter>(OtherActor);
	if (!OtherCharacter)
	{
		return false;
	}

	const FGameplayTag MyTeamTag = OwnerCharacter->GetTeamTag();
	const FGameplayTag OtherTeamTag = OtherCharacter->GetTeamTag();

	if (!MyTeamTag.IsValid() || !OtherTeamTag.IsValid())
	{
		return false;
	}

	return MyTeamTag == OtherTeamTag;
}

FDamageResult UCombatComponent::ApplyDamageRequest(const FDamageRequest& InRequest)
{
	FDamageResult Result;

	if (!ValidateDamageRequest(InRequest))
	{
		DebugPrint(TEXT("[CombatComponent] ApplyDamageRequest failed | ValidateDamageRequest returned false"));
		return Result;
	}

	if (ShouldIgnoreDamage(InRequest))
	{
		DebugPrint(TEXT("[CombatComponent] ApplyDamageRequest ignored | ShouldIgnoreDamage returned true"));
		return Result;
	}

	// Protego / Parry 판정
	if (TryHandleProtegoDefense(InRequest, Result))
	{
		HandleDamageResult(InRequest, Result);
		return Result;
	}

	if (!ApplyDamageEffect(InRequest, Result))
	{
		DebugPrint(TEXT("[CombatComponent] ApplyDamageRequest failed | ApplyDamageEffect returned false"));
		return Result;
	}

	HandleDamageResult(InRequest, Result);

	return Result;
}

bool UCombatComponent::ValidateDamageRequest(const FDamageRequest& InRequest) const
{
	if (!OwnerCharacter.IsValid())
	{
		return false;
	}

	if (!AbilitySystemComponent.IsValid())
	{
		return false;
	}

	if (!InRequest.SourceActor)
	{
		return false;
	}

	if (!InRequest.TargetActor)
	{
		return false;
	}

	if (InRequest.TargetActor != OwnerCharacter.Get())
	{
		return false;
	}

	if (InRequest.BaseDamage < 0.0f)
	{
		return false;
	}

	if (!DefaultDamageEffectClass)
	{
		return false;
	}

	return true;
}

bool UCombatComponent::ShouldIgnoreDamage(const FDamageRequest& InRequest) const
{
	if (!CanReceiveDamage())
	{
		return true;
	}

	if (!InRequest.SourceActor || !InRequest.TargetActor)
	{
		return true;
	}

	if (InRequest.SourceActor == InRequest.TargetActor)
	{
		return true;
	}

	if (!bAllowFriendlyFire && IsFriendlyTo(InRequest.SourceActor))
	{
		return true;
	}

	return false;
}

FGameplayEffectContextHandle UCombatComponent::BuildEffectContext(const FDamageRequest& InRequest) const
{
	FGameplayEffectContextHandle ContextHandle;

	if (!AbilitySystemComponent.IsValid())
	{
		return ContextHandle;
	}

	ContextHandle = AbilitySystemComponent->MakeEffectContext();

	AActor* InstigatorActor = InRequest.InstigatorActor.Get();
	if (!InstigatorActor)
	{
		InstigatorActor = InRequest.SourceActor.Get();
	}

	ContextHandle.AddInstigator(InstigatorActor, InRequest.DamageCauser.Get());

	if (InRequest.HitResult.bBlockingHit)
	{
		ContextHandle.AddHitResult(InRequest.HitResult);
	}

	if (InRequest.DamageCauser.Get())
	{
		ContextHandle.AddSourceObject(InRequest.DamageCauser.Get());
	}

	return ContextHandle;
}

bool UCombatComponent::ApplyDamageEffect(const FDamageRequest& InRequest, FDamageResult& OutResult)
{
	if (!AbilitySystemComponent.IsValid())
	{
		return false;
	}

	if (!DefaultDamageEffectClass)
	{
		return false;
	}

	const FGameplayEffectContextHandle ContextHandle = BuildEffectContext(InRequest);
	FGameplayEffectSpecHandle SpecHandle =
		AbilitySystemComponent->MakeOutgoingSpec(DefaultDamageEffectClass, 1.0f, ContextHandle);

	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		DebugPrint(TEXT("[CombatComponent] ApplyDamageEffect failed | SpecHandle invalid"));
		return false;
	}

	static const FGameplayTag DamageDataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"));

	SpecHandle.Data->SetSetByCallerMagnitude(DamageDataTag, InRequest.BaseDamage);

	if (InRequest.DamageTypeTag.IsValid())
	{
		SpecHandle.Data->AddDynamicAssetTag(InRequest.DamageTypeTag);
	}

	const FActiveGameplayEffectHandle ActiveHandle =
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	if (!ActiveHandle.IsValid())
	{
		DebugPrint(TEXT("[CombatComponent] ApplyDamageEffect failed | ApplyGameplayEffectSpecToSelf failed"));
		return false;
	}

	OutResult.bWasApplied = true;
	OutResult.FinalDamage = InRequest.BaseDamage;

	DebugPrint(FString::Printf(
		TEXT("[CombatComponent] DamageApplied | Target=%s | Source=%s | BaseDamage=%.2f"),
		*GetNameSafe(InRequest.TargetActor),
		*GetNameSafe(InRequest.SourceActor),
		InRequest.BaseDamage
	));

	return true;
}

void UCombatComponent::HandleDamageResult(const FDamageRequest& InRequest, FDamageResult& OutResult)
{
	LastDamageTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f;

	AActor* ResolvedInstigator = InRequest.InstigatorActor.Get();
	if (!ResolvedInstigator)
	{
		ResolvedInstigator = InRequest.SourceActor.Get();
	}
	LastHitInstigator = ResolvedInstigator;

	AActor* ResolvedCauser = InRequest.DamageCauser.Get();
	if (!ResolvedCauser)
	{
		ResolvedCauser = InRequest.SourceActor.Get();
	}
	LastHitCauser = ResolvedCauser;

	const UHOGAttributeSet* CurrentAttributeSet = GetAttributeSet();
	if (!CurrentAttributeSet)
	{
		DebugPrint(TEXT("[CombatComponent] HandleDamageResult warning | AttributeSet is null"));
		OnDamageApplied.Broadcast(OutResult);
		return;
	}

	if (CurrentAttributeSet->GetHealth() <= 0.0f)
	{
		OutResult.bKilledTarget = true;
		bIsDead = true;

		HandleDeath();
	}

	OnDamageApplied.Broadcast(OutResult);
}

void UCombatComponent::HandleDeath()
{
	if (!OwnerCharacter.IsValid())
	{
		return;
	}

	if (OwnerCharacter->IsDead())
	{
		bIsDead = true;
		OnDeath.Broadcast();
		return;
	}

	DebugPrint(FString::Printf(
		TEXT("[CombatComponent] HandleDeath | Owner=%s"),
		*GetNameSafe(OwnerCharacter.Get())
	));

	bIsDead = true;
	OwnerCharacter->Die();
	OnDeath.Broadcast();
}

void UCombatComponent::DebugPrint(const FString& Message) const
{
	if (!bEnableDebug)
	{
		return;
	}

	// Debug::Print(Message);
}

void UCombatComponent::OpenProtegoParryWindow(float DurationSeconds)
{
	if (DurationSeconds <= 0.0f)
	{
		ProtegoParryWindowEndTime = -1.0f;
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		ProtegoParryWindowEndTime = -1.0f;
		return;
	}

	ProtegoParryWindowEndTime = World->GetTimeSeconds() + DurationSeconds;

	DebugPrint(FString::Printf(
		TEXT("[CombatComponent] OpenProtegoParryWindow | EndTime=%.2f | Duration=%.2f"),
		ProtegoParryWindowEndTime,
		DurationSeconds
	));
}

void UCombatComponent::ClearProtegoParryWindow()
{
	ProtegoParryWindowEndTime = -1.0f;

	DebugPrint(TEXT("[CombatComponent] ClearProtegoParryWindow"));
}

bool UCombatComponent::IsProtegoParryWindowActive() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (ProtegoParryWindowEndTime < 0.0f)
	{
		return false;
	}

	return World->GetTimeSeconds() <= ProtegoParryWindowEndTime;
}

bool UCombatComponent::HasOwnerGameplayTag(const FGameplayTag& Tag) const
{
	if (!Tag.IsValid())
	{
		return false;
	}

	if (!AbilitySystemComponent.IsValid())
	{
		return false;
	}

	return AbilitySystemComponent->HasMatchingGameplayTag(Tag);
}

bool UCombatComponent::TryHandleProtegoDefense(const FDamageRequest& InRequest, FDamageResult& OutResult)
{
	// Protego 활성 상태가 아니면 여기서 처리 안 함
	if (!HasOwnerGameplayTag(HOGGameplayTags::State_Spell_Protego_Active))
	{
		return false;
	}

	// 1) 패링 성공
	if (IsProtegoParryWindowActive())
	{
		OutResult.bWasApplied = false;
		OutResult.bWasBlocked = true;
		OutResult.bWasParried = true;
		OutResult.bKilledTarget = false;
		OutResult.FinalDamage = 0.0f;

		if (bConsumeParryWindowOnSuccess)
		{
			ClearProtegoParryWindow();
		}

		AActor* AttackerActor = InRequest.SourceActor.Get();

		DebugPrint(FString::Printf(
			TEXT("[CombatComponent] Protego Parry Success | Target=%s | Attacker=%s"),
			*GetNameSafe(InRequest.TargetActor),
			*GetNameSafe(AttackerActor)
		));

		OnParrySuccess.Broadcast(AttackerActor);
		return true;
	}

	// 2) 일반 블록
	OutResult.bWasApplied = false;
	OutResult.bWasBlocked = true;
	OutResult.bWasParried = false;
	OutResult.bKilledTarget = false;
	OutResult.FinalDamage = 0.0f;

	DebugPrint(FString::Printf(
		TEXT("[CombatComponent] Protego Block | Target=%s | Source=%s"),
		*GetNameSafe(InRequest.TargetActor),
		*GetNameSafe(InRequest.SourceActor)
	));

	return true;
}