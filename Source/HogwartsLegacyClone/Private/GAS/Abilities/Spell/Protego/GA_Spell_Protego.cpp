#include "GAS/Abilities/Spell/Protego/GA_Spell_Protego.h"

#include "AbilitySystemComponent.h"
#include "Component/CombatComponent.h"
#include "GAS/Abilities/Spell/Protego/GE_Protego.h"
#include "GAS/Abilities/Spell/Protego/ProtegoActor.h"
#include "GAS/Abilities/Spell/Stupefy/GA_Spell_Stupefy.h"
#include "GameFramework/Actor.h"
#include "Character/BaseCharacter.h"
#include "GameFramework/HOG_GameState.h"
#include "Engine/World.h"
#include "HOGDebugHelper.h"

UGA_Spell_Protego::UGA_Spell_Protego()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Spell_Protego::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!AvatarActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 일반 시전 쿨타임 체크는 SpellComponent 정책을 탄다.
	{
		FSpellCastCheckResult CheckResult;
		if (!CanCastAsNormal(CheckResult))
		{
			const FSpellCastRequest FailedRequest = BuildSpellCastRequest(ESpellCastContext::Normal);
			NotifySpellCastFailedResult(FailedRequest, CheckResult);

			Debug::Print(TEXT("[GA_Spell_Protego] CanCastAsNormal failed"), FColor::Red);
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}

	// 코스트/커밋
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1) Protego 활성 GE 적용
	if (ProtegoEffectClass)
	{
		const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(
			ProtegoEffectClass,
			GetAbilityLevel(Handle, ActorInfo)
		);

		if (SpecHandle.IsValid())
		{
			ActiveProtegoEffectHandle = ApplyGameplayEffectSpecToOwner(
				Handle,
				ActorInfo,
				ActivationInfo,
				SpecHandle
			);
		}
		else
		{
			Debug::Print(TEXT("[GA_Spell_Protego] ProtegoEffect SpecHandle invalid"), FColor::Red);
		}
	}
	else
	{
		Debug::Print(TEXT("[GA_Spell_Protego] ProtegoEffectClass is null"), FColor::Red);
	}

	// 2) Protego Actor 스폰
	if (ProtegoActorClass)
	{
		UWorld* World = AvatarActor->GetWorld();
		if (World)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = AvatarActor;
			SpawnParams.Instigator = Cast<APawn>(AvatarActor);
			SpawnParams.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			SpawnedProtegoActor = World->SpawnActor<AProtegoActor>(
				ProtegoActorClass,
				AvatarActor->GetActorLocation(),
				AvatarActor->GetActorRotation(),
				SpawnParams
			);

			if (SpawnedProtegoActor)
			{
				SpawnedProtegoActor->AttachToActor(
					AvatarActor,
					FAttachmentTransformRules::SnapToTargetNotIncludingScale
				);

				Debug::Print(TEXT("[GA_Spell_Protego] ProtegoActor spawned"), FColor::Green);
			}
			else
			{
				Debug::Print(TEXT("[GA_Spell_Protego] Failed to spawn ProtegoActor"), FColor::Red);
			}
		}
	}
	else
	{
		Debug::Print(TEXT("[GA_Spell_Protego] ProtegoActorClass is null"), FColor::Yellow);
	}

	// 3) CombatComponent 패링 윈도우 오픈 + 이벤트 바인딩
	CachedCombatComponent = GetOwnerCombatComponent();
	if (CachedCombatComponent)
	{
		CachedCombatComponent->OpenProtegoParryWindow(ParryWindowSeconds);
		BindCombatDelegates();

		Debug::Print(
			FString::Printf(TEXT("[GA_Spell_Protego] Parry Window Opened | Duration=%.2f"), ParryWindowSeconds),
			FColor::Cyan
		);
	}
	else
	{
		Debug::Print(TEXT("[GA_Spell_Protego] CombatComponent not found"), FColor::Red);
	}

	// 4) 일반 시전 성공 통지 -> 쿨타임 시작
	NotifySpellCastSucceeded(ESpellCastContext::Normal);

	// 현재는 수동 종료 전까지 유지
}

void UGA_Spell_Protego::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UnbindCombatDelegates();

	if (CachedCombatComponent)
	{
		CachedCombatComponent->ClearProtegoParryWindow();
		CachedCombatComponent = nullptr;
	}

	// 1) 스폰한 ProtegoActor 정리
	if (SpawnedProtegoActor)
	{
		SpawnedProtegoActor->Destroy();
		SpawnedProtegoActor = nullptr;
	}

	// 2) 적용한 GE 제거
	if (ActiveProtegoEffectHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			ASC->RemoveActiveGameplayEffect(ActiveProtegoEffectHandle);
		}

		ActiveProtegoEffectHandle.Invalidate();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Spell_Protego::HandleParrySuccess(AActor* AttackerActor)
{
	Debug::Print(
		FString::Printf(TEXT("[GA_Spell_Protego] HandleParrySuccess | Attacker=%s"), *GetNameSafe(AttackerActor)),
		FColor::Yellow
	);

	TryTriggerCounterStupefy(AttackerActor);
}

UCombatComponent* UGA_Spell_Protego::GetOwnerCombatComponent() const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return nullptr;
	}

	return AvatarActor->FindComponentByClass<UCombatComponent>();
}

bool UGA_Spell_Protego::TryTriggerCounterStupefy(AActor* AttackerActor)
{
	if (!IsValid(AttackerActor))
	{
		Debug::Print(TEXT("[GA_Spell_Protego] TryTriggerCounterStupefy failed | AttackerActor invalid"), FColor::Red);
		return false;
	}

	if (!CounterStupefyAbilityClass)
	{
		Debug::Print(TEXT("[GA_Spell_Protego] CounterStupefyAbilityClass is null"), FColor::Red);
		return false;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		Debug::Print(TEXT("[GA_Spell_Protego] ASC missing"), FColor::Red);
		return false;
	}

	FGameplayAbilitySpec* FoundSpec = nullptr;

	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.Ability)
		{
			continue;
		}

		if (Spec.Ability->GetClass() == CounterStupefyAbilityClass)
		{
			FoundSpec = &Spec;
			break;
		}
	}

	if (!FoundSpec)
	{
		Debug::Print(TEXT("[GA_Spell_Protego] Counter Stupefy spec not found on ASC"), FColor::Red);
		return false;
	}

	UGA_SpellStupefy* StupefyInstance = Cast<UGA_SpellStupefy>(FoundSpec->GetPrimaryInstance());
	if (!StupefyInstance)
	{
		Debug::Print(TEXT("[GA_Spell_Protego] Stupefy primary instance missing"), FColor::Red);
		return false;
	}

	StupefyInstance->PrepareCastContext(ESpellCastContext::ParryCounter, AttackerActor);

	const bool bActivated = ASC->TryActivateAbility(FoundSpec->Handle);

	Debug::Print(
		FString::Printf(TEXT("[GA_Spell_Protego] TryTriggerCounterStupefy | Activated=%d"), bActivated ? 1 : 0),
		bActivated ? FColor::Green : FColor::Red
	);

	return bActivated;
}

void UGA_Spell_Protego::BindCombatDelegates()
{
	if (!CachedCombatComponent)
	{
		return;
	}

	CachedCombatComponent->OnParrySuccess.RemoveDynamic(this, &UGA_Spell_Protego::HandleParrySuccess);
	CachedCombatComponent->OnParrySuccess.AddDynamic(this, &UGA_Spell_Protego::HandleParrySuccess);
}

void UGA_Spell_Protego::UnbindCombatDelegates()
{
	if (!CachedCombatComponent)
	{
		return;
	}

	CachedCombatComponent->OnParrySuccess.RemoveDynamic(this, &UGA_Spell_Protego::HandleParrySuccess);
}