#include "Component/CombatComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GAS/Attributes/HOGAttributeSet.h"
#include "Character/BaseCharacter.h"
#include "Core/HOG_GameplayTags.h"
#include "GameplayEffect.h"
#include "GameFramework/Pawn.h"
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
		return;
	}

	OwnerCharacter = Cast<ABaseCharacter>(MyOwner);
	if (!OwnerCharacter.IsValid())
	{
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
}

UAbilitySystemComponent* UCombatComponent::GetAbilitySystemComponent() const
{
	if (AbilitySystemComponent.IsValid())
	{
		return AbilitySystemComponent.Get();
	}

	AActor* MyOwner = GetOwner();
	if (!MyOwner)
	{
		return nullptr;
	}

	UAbilitySystemComponent* FoundASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MyOwner);
	if (FoundASC)
	{
		return FoundASC;
	}

	if (const APawn* OwnerPawn = Cast<APawn>(MyOwner))
	{
		if (APlayerState* PS = OwnerPawn->GetPlayerState())
		{
			return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS);
		}
	}

	return nullptr;
}

const UHOGAttributeSet* UCombatComponent::GetAttributeSet() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return nullptr;
	}

	return ASC->GetSet<UHOGAttributeSet>();
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

	if (!GetAbilitySystemComponent())
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
		return Result;
	}

	if (ShouldIgnoreDamage(InRequest))
	{
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

	if (!GetAbilitySystemComponent())
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

FGameplayEffectContextHandle UCombatComponent::BuildEffectContext(
	const FDamageRequest& InRequest,
	UAbilitySystemComponent* EffectSourceASC
) const
{
	FGameplayEffectContextHandle ContextHandle;

	if (!EffectSourceASC)
	{
		return ContextHandle;
	}

	ContextHandle = EffectSourceASC->MakeEffectContext();

	AActor* InstigatorActor = InRequest.SourceActor.Get();
	if (!InstigatorActor)
	{
		InstigatorActor = InRequest.InstigatorActor.Get();
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

float UCombatComponent::CalculateExpectedFinalDamage(const FDamageRequest& InRequest) const
{
	float SourceAttackPower = 0.0f;

	if (AActor* SourceActor = InRequest.SourceActor.Get())
	{
		if (UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceActor))
		{
			if (const UHOGAttributeSet* SourceAttributeSet = SourceASC->GetSet<UHOGAttributeSet>())
			{
				SourceAttackPower = SourceAttributeSet->GetAttackPower();
			}
		}
		else if (const APawn* SourcePawn = Cast<APawn>(SourceActor))
		{
			if (APlayerState* SourcePS = SourcePawn->GetPlayerState())
			{
				if (UAbilitySystemComponent* SourceASCFromPS =
					UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourcePS))
				{
					if (const UHOGAttributeSet* SourceAttributeSet = SourceASCFromPS->GetSet<UHOGAttributeSet>())
					{
						SourceAttackPower = SourceAttributeSet->GetAttackPower();
					}
				}
			}
		}
	}

	return FMath::Max(InRequest.BaseDamage + SourceAttackPower, 0.0f);
}

bool UCombatComponent::ApplyDamageEffect(const FDamageRequest& InRequest, FDamageResult& OutResult)
{
	UAbilitySystemComponent* TargetASC = GetAbilitySystemComponent();
	if (!TargetASC)
	{
		return false;
	}

	if (!DefaultDamageEffectClass)
	{
		return false;
	}

	AActor* SourceActor = InRequest.SourceActor.Get();
	if (!SourceActor)
	{
		return false;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceActor);

	// SourceActor가 Pawn이고 ASC가 PlayerState에 붙어있는 경우 재시도
	if (!SourceASC)
	{
		if (const APawn* SourcePawn = Cast<APawn>(SourceActor))
		{
			if (APlayerState* SourcePS = SourcePawn->GetPlayerState())
			{
				SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourcePS);
			}
		}
	}

	if (!SourceASC)
	{
		return false;
	}

	const float ExpectedFinalDamage = CalculateExpectedFinalDamage(InRequest);
	const FGameplayEffectContextHandle ContextHandle = BuildEffectContext(InRequest, SourceASC);

	// Spec은 공격자(Source) ASC에서 만들고,
	// 적용은 피격자(Target) ASC에 한다.
	FGameplayEffectSpecHandle SpecHandle =
		SourceASC->MakeOutgoingSpec(DefaultDamageEffectClass, 1.0f, ContextHandle);

	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		return false;
	}

	const FGameplayTag DamageDataTag = HOGGameplayTags::Data_Damage;
	SpecHandle.Data->SetSetByCallerMagnitude(DamageDataTag, InRequest.BaseDamage);

	if (InRequest.DamageTypeTag.IsValid())
	{
		SpecHandle.Data->AddDynamicAssetTag(InRequest.DamageTypeTag);
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	OutResult.bWasApplied = true;
	OutResult.FinalDamage = ExpectedFinalDamage;

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
		OnDamageApplied.Broadcast(OutResult);
		return;
	}

	if (CurrentAttributeSet->GetHealth() <= 0.0f)
	{
		OutResult.bKilledTarget = true;
		bIsDead = true;

		HandleDeath();
	}
	else if (OutResult.bWasApplied && !OutResult.bWasBlocked)
	{
		UAbilitySystemComponent* TargetASC = GetAbilitySystemComponent();
		if (TargetASC)
		{
			FGameplayTagContainer HitReactTag;
			HitReactTag.AddTag(HOGGameplayTags::State_Hit);
			TargetASC->TryActivateAbilitiesByTag(HitReactTag);
		}
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

	bIsDead = true;
	OwnerCharacter->Die();
	OnDeath.Broadcast();
}

void UCombatComponent::DebugPrint(const FString& Message) const
{
}

void UCombatComponent::OpenProtegoDefenseWindow(float ParryDurationSeconds, float BlockDurationSeconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		ProtegoParryWindowEndTime = -1.0f;
		ProtegoBlockWindowEndTime = -1.0f;
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();

	ProtegoParryWindowEndTime = (ParryDurationSeconds > 0.0f)
		? (CurrentTime + ParryDurationSeconds)
		: -1.0f;

	ProtegoBlockWindowEndTime = (BlockDurationSeconds > 0.0f)
		? (CurrentTime + BlockDurationSeconds)
		: -1.0f;
}

void UCombatComponent::ClearProtegoDefenseWindow()
{
	ProtegoParryWindowEndTime = -1.0f;
	ProtegoBlockWindowEndTime = -1.0f;
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

bool UCombatComponent::IsProtegoBlockWindowActive() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (ProtegoBlockWindowEndTime < 0.0f)
	{
		return false;
	}

	return World->GetTimeSeconds() <= ProtegoBlockWindowEndTime;
}

bool UCombatComponent::HasOwnerGameplayTag(const FGameplayTag& Tag) const
{
	if (!Tag.IsValid())
	{
		return false;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	return ASC->HasMatchingGameplayTag(Tag);
}

bool UCombatComponent::TryHandleProtegoDefense(const FDamageRequest& InRequest, FDamageResult& OutResult)
{
	if (!HasOwnerGameplayTag(HOGGameplayTags::State_Spell_Protego_Active))
	{
		return false;
	}

	// 블록 시간도 끝났으면 Protego 방어 처리 안 함
	if (!IsProtegoBlockWindowActive())
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
			ProtegoParryWindowEndTime = -1.0f;
		}

		AActor* AttackerActor = InRequest.SourceActor.Get();

		OnParrySuccess.Broadcast(AttackerActor);
		return true;
	}

	// 2) 일반 블록
	OutResult.bWasApplied = false;
	OutResult.bWasBlocked = true;
	OutResult.bWasParried = false;
	OutResult.bKilledTarget = false;
	OutResult.FinalDamage = 0.0f;

	return true;
}