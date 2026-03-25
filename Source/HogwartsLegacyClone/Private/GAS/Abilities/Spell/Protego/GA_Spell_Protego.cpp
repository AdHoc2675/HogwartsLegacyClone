#include "GAS/Abilities/Spell/Protego/GA_Spell_Protego.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemInterface.h"

#include "Component/CombatComponent.h"

#include "GAS/Abilities/Spell/Protego/GE_Protego.h"
#include "GAS/Abilities/Spell/Protego/ProtegoActor.h"
#include "GAS/Abilities/Spell/Stupefy/GA_Spell_Stupefy.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Character.h"

#include "Core/HOG_GameplayTags.h"
#include "Character/BaseCharacter.h"
#include "GameFramework/HOG_GameState.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#include "Engine/World.h"
#include "TimerManager.h"

#include "GameFramework/HOG_PlayerController.h"
#include "UI/HOG_WidgetController.h"

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
	
	ACharacter* Character = Cast<ACharacter>(AvatarActor);

	if (CastVoiceSound && Character)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastVoiceSound, Character->GetActorLocation());
		
		// =============== [자막 호출] ===============
		if (AHOG_PlayerController* PC = Cast<AHOG_PlayerController>(Character->GetController()))
		{
			if (UHOG_WidgetController* UIController = PC->GetWidgetController())
			{
				UIController->RequestSubtitle(CastSubtitleText, 1.0f);
			}
		}
	}

	if (CastSound && Character)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastSound, Character->GetActorLocation());
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
			}
		}
	}

	// 3) CombatComponent 패링 윈도우 오픈 + 이벤트 바인딩
	CachedCombatComponent = GetOwnerCombatComponent();
	if (CachedCombatComponent)
	{
		CachedCombatComponent->OpenProtegoDefenseWindow(ParryWindowSeconds, BlockWindowSeconds);
		BindCombatDelegates();
	}

	// 4) 일반 시전 성공 통지 -> 쿨타임 시작
	NotifySpellCastSucceeded(ESpellCastContext::Normal);

	// 5) 발동 애니메이션 재생 (연출용)
	if (Character && Character->GetMesh() && CastMontage)
	{
		if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
		{
			AnimInstance->Montage_Play(CastMontage, 1.0f);
		}
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ProtegoLifetimeTimerHandle);
		World->GetTimerManager().SetTimer(
			ProtegoLifetimeTimerHandle,
			[this]()
			{
				if (IsActive())
				{
					EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
				}
			},
			BlockWindowSeconds,
			false
		);
	}
}

void UGA_Spell_Protego::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UnbindCombatDelegates();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ProtegoLifetimeTimerHandle);
	}

	if (CachedCombatComponent)
	{
		CachedCombatComponent->ClearProtegoDefenseWindow();
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
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (ParrySuccessSound && AvatarActor)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ParrySuccessSound, AvatarActor->GetActorLocation());
	}
	
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
		return false;
	}

	if (!CounterStupefyAbilityClass)
	{
		return false;
	}

	if (!CanTriggerCounterStupefyFromAttacker(AttackerActor))
	{
		return false;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
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
		return false;
	}

	UGA_SpellStupefy* StupefyInstance = Cast<UGA_SpellStupefy>(FoundSpec->GetPrimaryInstance());
	if (!StupefyInstance)
	{
		return false;
	}

	StupefyInstance->PrepareCastContext(ESpellCastContext::ParryCounter, AttackerActor);

	return ASC->TryActivateAbility(FoundSpec->Handle);
}

bool UGA_Spell_Protego::CanTriggerCounterStupefyFromAttacker(AActor* AttackerActor) const
{
	UAbilitySystemComponent* AttackerASC = ResolveAbilitySystemComponentFromActor(AttackerActor);
	if (!AttackerASC)
	{
		return false;
	}

	const FGameplayTagContainer& OwnedTags = AttackerASC->GetOwnedGameplayTags();

	return OwnedTags.HasTag(HOGGameplayTags::Team_Enemy) ||
		   OwnedTags.HasTag(FGameplayTag::RequestGameplayTag(FName("Unit.Enemy")));
}

UAbilitySystemComponent* UGA_Spell_Protego::ResolveAbilitySystemComponentFromActor(AActor* InActor) const
{
	if (!InActor)
	{
		return nullptr;
	}

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(InActor))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			return ASC;
		}
	}

	if (APawn* Pawn = Cast<APawn>(InActor))
	{
		if (APlayerState* PS = Pawn->GetPlayerState())
		{
			if (IAbilitySystemInterface* PSASI = Cast<IAbilitySystemInterface>(PS))
			{
				return PSASI->GetAbilitySystemComponent();
			}
		}
	}

	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InActor);
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

bool UGA_Spell_Protego::ShouldApplyCastingActiveTag() const
{
	return false;
}