#include "Component/CombatComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GAS/Attributes/HOGAttributeSet.h"
#include "Character/BaseCharacter.h"
#include "HOGDebugHelper.h"
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
		DebugPrint(TEXT("[CombatComponent] ValidateDamageRequest failed | OwnerCharacter invalid"));
		return false;
	}

	if (!GetAbilitySystemComponent())
	{
		DebugPrint(FString::Printf(
			TEXT("[CombatComponent] ValidateDamageRequest failed | ASC missing | Owner=%s"),
			*GetNameSafe(OwnerCharacter.Get())
		));
		return false;
	}

	if (!InRequest.SourceActor)
	{
		DebugPrint(FString::Printf(
			TEXT("[CombatComponent] ValidateDamageRequest failed | SourceActor null | Owner=%s"),
			*GetNameSafe(OwnerCharacter.Get())
		));
		return false;
	}

	if (!InRequest.TargetActor)
	{
		DebugPrint(FString::Printf(
			TEXT("[CombatComponent] ValidateDamageRequest failed | TargetActor null | Owner=%s | Source=%s"),
			*GetNameSafe(OwnerCharacter.Get()),
			*GetNameSafe(InRequest.SourceActor.Get())
		));
		return false;
	}

	if (InRequest.TargetActor != OwnerCharacter.Get())
	{
		DebugPrint(FString::Printf(
			TEXT(
				"[CombatComponent] ValidateDamageRequest failed | Target mismatch | Owner=%s | RequestTarget=%s | Source=%s"),
			*GetNameSafe(OwnerCharacter.Get()),
			*GetNameSafe(InRequest.TargetActor.Get()),
			*GetNameSafe(InRequest.SourceActor.Get())
		));
		return false;
	}

	if (InRequest.BaseDamage < 0.0f)
	{
		DebugPrint(FString::Printf(
			TEXT("[CombatComponent] ValidateDamageRequest failed | BaseDamage negative | Owner=%s | BaseDamage=%.2f"),
			*GetNameSafe(OwnerCharacter.Get()),
			InRequest.BaseDamage
		));
		return false;
	}

	if (!DefaultDamageEffectClass)
	{
		DebugPrint(FString::Printf(
			TEXT("[CombatComponent] ValidateDamageRequest failed | DefaultDamageEffectClass null | Owner=%s"),
			*GetNameSafe(OwnerCharacter.Get())
		));
		return false;
	}

	DebugPrint(FString::Printf(
		TEXT("[CombatComponent] ValidateDamageRequest success | Owner=%s | Source=%s | Target=%s | BaseDamage=%.2f"),
		*GetNameSafe(OwnerCharacter.Get()),
		*GetNameSafe(InRequest.SourceActor.Get()),
		*GetNameSafe(InRequest.TargetActor.Get()),
		InRequest.BaseDamage
	));

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
		DebugPrint(TEXT("[CombatComponent] BuildEffectContext failed | EffectSourceASC is null"));
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

	DebugPrint(FString::Printf(
		TEXT("[CombatComponent] BuildEffectContext | SourceASCOwner=%s | Instigator=%s | Causer=%s"),
		*GetNameSafe(EffectSourceASC->GetOwner()),
		*GetNameSafe(InstigatorActor),
		*GetNameSafe(InRequest.DamageCauser.Get())
	));

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
		DebugPrint(TEXT("[CombatComponent] ApplyDamageEffect failed | TargetASC is null"));
		return false;
	}

	if (!DefaultDamageEffectClass)
	{
		DebugPrint(TEXT("[CombatComponent] ApplyDamageEffect failed | DefaultDamageEffectClass is null"));
		return false;
	}

	AActor* SourceActor = InRequest.SourceActor.Get();
	if (!SourceActor)
	{
		DebugPrint(TEXT("[CombatComponent] ApplyDamageEffect failed | SourceActor is null"));
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
		DebugPrint(FString::Printf(
			TEXT("[CombatComponent] ApplyDamageEffect failed | SourceASC is null | Source=%s"),
			*GetNameSafe(SourceActor)
		));
		return false;
	}
	
	if (SourceASC->AbilityActorInfo.IsValid())
	{
		DebugPrint(FString::Printf(
			TEXT("[CombatComponent] SourceASC ActorInfo | ASC=%s | Owner=%s | Avatar=%s"),
			*GetNameSafe(SourceASC->GetOwner()),
			*GetNameSafe(SourceASC->AbilityActorInfo->OwnerActor.Get()),
			*GetNameSafe(SourceASC->AbilityActorInfo->AvatarActor.Get())
		));
	}
	else
	{
		DebugPrint(FString::Printf(
			TEXT("[CombatComponent] SourceASC ActorInfo INVALID | ASC=%s"),
			*GetNameSafe(SourceASC->GetOwner())
		));
	}

	if (TargetASC->AbilityActorInfo.IsValid())
	{
		DebugPrint(FString::Printf(
			TEXT("[CombatComponent] TargetASC ActorInfo | ASC=%s | Owner=%s | Avatar=%s"),
			*GetNameSafe(TargetASC->GetOwner()),
			*GetNameSafe(TargetASC->AbilityActorInfo->OwnerActor.Get()),
			*GetNameSafe(TargetASC->AbilityActorInfo->AvatarActor.Get())
		));
	}
	else
	{
		DebugPrint(FString::Printf(
			TEXT("[CombatComponent] TargetASC ActorInfo INVALID | ASC=%s"),
			*GetNameSafe(TargetASC->GetOwner())
		));
	}

	const UHOGAttributeSet* SourceAttributeSet = SourceASC->GetSet<UHOGAttributeSet>();
	const UHOGAttributeSet* TargetAttributeSet = TargetASC->GetSet<UHOGAttributeSet>();

	DebugPrint(FString::Printf(
		TEXT(
			"[CombatComponent] ApplyDamageEffect ASC Check | SourceActor=%s | SourceASC=%s | TargetOwner=%s | TargetASC=%s"),
		*GetNameSafe(SourceActor),
		*GetNameSafe(SourceASC->GetOwner()),
		*GetNameSafe(InRequest.TargetActor.Get()),
		*GetNameSafe(TargetASC->GetOwner())
	));

	DebugPrint(FString::Printf(
		TEXT("[CombatComponent] ApplyDamageEffect AttributeSet Check | SourceAttr=%s | TargetAttr=%s"),
		SourceAttributeSet ? TEXT("Valid") : TEXT("Null"),
		TargetAttributeSet ? TEXT("Valid") : TEXT("Null")
	));

	if (SourceAttributeSet)
	{
		DebugPrint(FString::Printf(
			TEXT("[CombatComponent] ApplyDamageEffect Source AttackPower = %.2f"),
			SourceAttributeSet->GetAttackPower()
		));
	}
	else
	{
		DebugPrint(TEXT("[CombatComponent] ApplyDamageEffect SourceAttributeSet is null"));
	}

	if (TargetAttributeSet)
	{
		DebugPrint(FString::Printf(
			TEXT("[CombatComponent] ApplyDamageEffect Target Health = %.2f | MaxHealth = %.2f"),
			TargetAttributeSet->GetHealth(),
			TargetAttributeSet->GetMaxHealth()
		));
	}
	else
	{
		DebugPrint(TEXT("[CombatComponent] ApplyDamageEffect TargetAttributeSet is null"));
	}

	const float ExpectedFinalDamage = CalculateExpectedFinalDamage(InRequest);

	const FGameplayEffectContextHandle ContextHandle = BuildEffectContext(InRequest, SourceASC);

	// 핵심:
	// Spec은 공격자(Source) ASC에서 만들고,
	// 적용은 피격자(Target) ASC에 한다.
	FGameplayEffectSpecHandle SpecHandle =
		SourceASC->MakeOutgoingSpec(DefaultDamageEffectClass, 1.0f, ContextHandle);

	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		DebugPrint(TEXT("[CombatComponent] ApplyDamageEffect failed | SpecHandle invalid"));
		return false;
	}

	const FGameplayTag DamageDataTag = HOGGameplayTags::Data_Damage;
	SpecHandle.Data->SetSetByCallerMagnitude(DamageDataTag, InRequest.BaseDamage);

	if (InRequest.DamageTypeTag.IsValid())
	{
		SpecHandle.Data->AddDynamicAssetTag(InRequest.DamageTypeTag);
	}

	// 적용은 공격자 SourceASC가 TargetASC에게 수행
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	OutResult.bWasApplied = true;
	OutResult.FinalDamage = ExpectedFinalDamage;

	DebugPrint(FString::Printf(
		TEXT("[CombatComponent] DamageApplied | Target=%s | Source=%s | BaseDamage=%.2f | FinalDamage=%.2f"),
		*GetNameSafe(InRequest.TargetActor),
		*GetNameSafe(InRequest.SourceActor),
		InRequest.BaseDamage,
		ExpectedFinalDamage
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
	// 데미지 적용이 되었고, 블록이 되지 않는 경우
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

	 Debug::Print(Message);
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

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	return ASC->HasMatchingGameplayTag(Tag);
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
