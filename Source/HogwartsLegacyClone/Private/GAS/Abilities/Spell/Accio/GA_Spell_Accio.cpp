#include "GAS/Abilities/Spell/Accio/GA_Spell_Accio.h"

#include "Core/HOG_GameplayTags.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HOG_PlayerController.h"
#include "UI/HOG_WidgetController.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "Character/Player/PlayerCharacterBase.h"

#include "Components/AudioComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"

UGA_Spell_Accio::UGA_Spell_Accio()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Spell_Accio::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	FGameplayTagContainer RelevantTags;
	if (!CheckCooldown(Handle, ActorInfo, &RelevantTags))
	{
		FinishAccioAbilityEnd(true, false);
		return;
	}

	if (TryBeginPreCastFacing(Handle, ActorInfo, ActivationInfo, TriggerEventData))
	{
		return;
	}

	ExecuteAccioCast(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Spell_Accio::OnPreCastFacingFinished(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ExecuteAccioCast(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Spell_Accio::ExecuteAccioCast(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bCastNotifyHandled = false;
	bPendingMontageEndTransition = false;
	bPendingEndAbilityReplicate = false;
	bPendingEndAbilityWasCancelled = false;

	ClearPersistentBeamVFX();

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		FinishAccioAbilityEnd(true, true);
		return;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);

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

	RegisterCastNotifyToOwner();

	if (Character && Character->GetMesh() && CastMontage)
	{
		if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
		{
			const float Duration = AnimInstance->Montage_Play(CastMontage, 1.0f);
			if (Duration > 0.f)
			{
				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &UGA_Spell_Accio::OnMontageEnded);
				AnimInstance->Montage_SetEndDelegate(EndDelegate, CastMontage);

				TryJumpMontageToSection(StartSectionName);
				return;
			}
		}
	}

	HandleCastNotify();
}

void UGA_Spell_Accio::InputPressed(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	if (IsValid(TargetToMove))
	{
		BeginMontageEndTransition(true, false);
	}
}

void UGA_Spell_Accio::HandleCastNotify()
{
	if (bCastNotifyHandled)
	{
		return;
	}

	bCastNotifyHandled = true;

	if (!FireAccio())
	{
		FinishAccioAbilityEnd(true, false);
		return;
	}

	NotifySpellCastSucceeded(ESpellCastContext::Normal);

	SpawnPersistentBeamVFX();

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character && Character->GetMesh() && HoldLoopMontage)
	{
		if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
		{
			if (CastMontage)
			{
				bIgnoreNextCastMontageInterrupted = true;
				AnimInstance->Montage_Stop(0.1f, CastMontage);
			}

			AnimInstance->Montage_Play(HoldLoopMontage, 1.0f);
		}
	}
}

bool UGA_Spell_Accio::TryJumpMontageToSection(FName SectionName) const
{
	if (SectionName.IsNone() || !CastMontage)
	{
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->GetMesh())
	{
		return false;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return false;
	}

	if (!AnimInstance->Montage_IsPlaying(CastMontage))
	{
		return false;
	}

	AnimInstance->Montage_JumpToSection(SectionName, CastMontage);
	return true;
}

void UGA_Spell_Accio::BeginMontageEndTransition(bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bPendingMontageEndTransition)
	{
		return;
	}

	bPendingMontageEndTransition = true;
	bPendingEndAbilityReplicate = bReplicateEndAbility;
	bPendingEndAbilityWasCancelled = bWasCancelled;

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->GetMesh())
	{
		FinishAccioAbilityEnd(bReplicateEndAbility, bWasCancelled);
		return;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance || !CastMontage)
	{
		FinishAccioAbilityEnd(bReplicateEndAbility, bWasCancelled);
		return;
	}

	if (HoldLoopMontage)
	{
		AnimInstance->Montage_Stop(0.1f, HoldLoopMontage);
	}

	const float Duration = AnimInstance->Montage_Play(CastMontage, 1.0f);
	if (Duration <= 0.f)
	{
		FinishAccioAbilityEnd(bReplicateEndAbility, bWasCancelled);
		return;
	}

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UGA_Spell_Accio::OnMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, CastMontage);

	AnimInstance->Montage_JumpToSection(EndSectionName, CastMontage);
}

void UGA_Spell_Accio::FinishAccioAbilityEnd(bool bReplicateEndAbility, bool bWasCancelled)
{
	EndAbility(
		CurrentSpecHandle,
		CurrentActorInfo,
		CurrentActivationInfo,
		bReplicateEndAbility,
		bWasCancelled
	);
}

bool UGA_Spell_Accio::SpawnPersistentBeamVFX()
{
	ClearPersistentBeamVFX();

	if (!AccioVFX)
	{
		return false;
	}

	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;

	if (!GetCurrentBeamStartLocation(StartLocation))
	{
		return false;
	}

	if (!GetCurrentBeamEndLocation(EndLocation))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FRotator SpawnRotation = (EndLocation - StartLocation).Rotation();

	ActiveBeamVFXComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		World,
		AccioVFX,
		StartLocation,
		SpawnRotation,
		FVector(1.f),
		true,
		true,
		ENCPoolMethod::None,
		true
	);

	if (!ActiveBeamVFXComponent)
	{
		return false;
	}

	ActiveBeamVFXComponent->SetVectorParameter(BeamStartParamName, StartLocation);
	ActiveBeamVFXComponent->SetVectorParameter(BeamEndParamName, EndLocation);
	ActiveBeamVFXComponent->SetFloatParameter(
		BeamLengthParamName,
		FVector::Distance(StartLocation, EndLocation)
	);

	return true;
}

void UGA_Spell_Accio::UpdatePersistentBeamVFX()
{
	if (!ActiveBeamVFXComponent)
	{
		return;
	}

	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;

	if (!GetCurrentBeamStartLocation(StartLocation) || !GetCurrentBeamEndLocation(EndLocation))
	{
		ClearPersistentBeamVFX();
		return;
	}

	ActiveBeamVFXComponent->SetWorldLocation(StartLocation);
	ActiveBeamVFXComponent->SetWorldRotation((EndLocation - StartLocation).Rotation());

	ActiveBeamVFXComponent->SetVectorParameter(BeamStartParamName, StartLocation);
	ActiveBeamVFXComponent->SetVectorParameter(BeamEndParamName, EndLocation);
	ActiveBeamVFXComponent->SetFloatParameter(
		BeamLengthParamName,
		FVector::Distance(StartLocation, EndLocation)
	);
}

void UGA_Spell_Accio::ClearPersistentBeamVFX()
{
	if (ActiveBeamVFXComponent)
	{
		ActiveBeamVFXComponent->Deactivate();
		ActiveBeamVFXComponent->DestroyComponent();
		ActiveBeamVFXComponent = nullptr;
	}
}

bool UGA_Spell_Accio::GetCurrentBeamStartLocation(FVector& OutStartLocation) const
{
	OutStartLocation = FVector::ZeroVector;

	AActor* Avatar = GetAvatarActorFromActorInfo();
	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(Avatar);
	if (!PlayerCharacter)
	{
		return false;
	}

	// 1) WandMesh(static mesh)의 소켓 우선 사용
	if (UStaticMeshComponent* WandMeshComp = PlayerCharacter->GetWandMesh())
	{
		if (BeamStartSocketName != NAME_None &&
			WandMeshComp->DoesSocketExist(BeamStartSocketName))
		{
			OutStartLocation = WandMeshComp->GetSocketLocation(BeamStartSocketName);
			return true;
		}
	}

	// 2) 기존 fallback : 캐릭터 SkeletalMesh 소켓
	if (USkeletalMeshComponent* MeshComp = PlayerCharacter->GetMesh())
	{
		if (BeamStartSocketName != NAME_None &&
			MeshComp->DoesSocketExist(BeamStartSocketName))
		{
			OutStartLocation = MeshComp->GetSocketLocation(BeamStartSocketName);
			return true;
		}
	}

	// 3) 최종 fallback : 캐릭터 위치
	OutStartLocation = PlayerCharacter->GetActorLocation();
	return true;
}

AActor* UGA_Spell_Accio::ResolveCurrentBeamTargetActor() const
{
	if (IsValid(TargetToMove))
	{
		return TargetToMove;
	}

	if (IsValid(OriginalTarget))
	{
		return OriginalTarget;
	}

	return nullptr;
}

UAbilitySystemComponent* UGA_Spell_Accio::GetBeamTargetASC(AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return nullptr;
	}

	return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
}

USceneComponent* UGA_Spell_Accio::GetBeamEndAttachComponent(AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return nullptr;
	}

	UAbilitySystemComponent* TargetASC = GetBeamTargetASC(TargetActor);

	// 1) ASC 태그 기준 우선 분기
	if (TargetASC)
	{
		FGameplayTagContainer OwnedTags;
		TargetASC->GetOwnedGameplayTags(OwnedTags);

		// Enemy -> SkeletalMesh 우선
		if (OwnedTags.HasTag(HOGGameplayTags::Team_Enemy) ||
			OwnedTags.HasTag(FGameplayTag::RequestGameplayTag(FName("Unit.Enemy"))))
		{
			if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
			{
				if (USkeletalMeshComponent* CharacterMesh = TargetCharacter->GetMesh())
				{
					return CharacterMesh;
				}
			}

			if (USkeletalMeshComponent* SkeletalMeshComp = TargetActor->FindComponentByClass<USkeletalMeshComponent>())
			{
				return SkeletalMeshComp;
			}
		}

		// Object -> StaticMesh 우선
		if (OwnedTags.HasTag(HOGGameplayTags::Team_Object) ||
			OwnedTags.HasTag(HOGGameplayTags::Interactable_AccioTarget) ||
			OwnedTags.HasTag(HOGGameplayTags::Interactable_AccioPlatform))
		{
			if (UStaticMeshComponent* StaticMeshComp = TargetActor->FindComponentByClass<UStaticMeshComponent>())
			{
				return StaticMeshComp;
			}
		}
	}

	// 2) fallback : 캐릭터면 SkeletalMesh
	if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
	{
		if (USkeletalMeshComponent* CharacterMesh = TargetCharacter->GetMesh())
		{
			return CharacterMesh;
		}
	}

	// 3) fallback : 물리 시뮬레이션 중인 Primitive
	TArray<UPrimitiveComponent*> PrimitiveComps;
	TargetActor->GetComponents<UPrimitiveComponent>(PrimitiveComps);

	for (UPrimitiveComponent* PrimComp : PrimitiveComps)
	{
		if (PrimComp && PrimComp->IsSimulatingPhysics())
		{
			return PrimComp;
		}
	}

	// 4) fallback : StaticMesh
	if (UStaticMeshComponent* StaticMeshComp = TargetActor->FindComponentByClass<UStaticMeshComponent>())
	{
		return StaticMeshComp;
	}

	// 5) fallback : SkeletalMesh
	if (USkeletalMeshComponent* SkeletalMeshComp = TargetActor->FindComponentByClass<USkeletalMeshComponent>())
	{
		return SkeletalMeshComp;
	}

	// 6) 최종 fallback : RootComponent
	return TargetActor->GetRootComponent();
}


bool UGA_Spell_Accio::GetCurrentBeamEndLocation(FVector& OutEndLocation) const
{
		OutEndLocation = FVector::ZeroVector;

	// =========================
	// 1) 시각 기준은 항상 원래 맞춘 타겟 우선
	//    - 발판 위에서 적에게 Accio를 쓴 경우에도
	//      레이저는 적(OriginalTarget)에 꽂혀야 한다.
	// =========================
	if (IsValid(OriginalTarget))
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OriginalTarget);

		if (TargetASC)
		{
			// Enemy -> Character SkeletalMesh
			if (TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Team_Enemy) ||
				TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Unit.Enemy"))))
			{
				if (ACharacter* TargetCharacter = Cast<ACharacter>(OriginalTarget))
				{
					if (USkeletalMeshComponent* CharacterMesh = TargetCharacter->GetMesh())
					{
						OutEndLocation = CharacterMesh->GetComponentLocation();
						return true;
					}
				}

				if (USkeletalMeshComponent* SkeletalMeshComp = OriginalTarget->FindComponentByClass<USkeletalMeshComponent>())
				{
					OutEndLocation = SkeletalMeshComp->GetComponentLocation();
					return true;
				}
			}

			// Object / Interactable -> StaticMesh
			if (TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Team_Object) ||
				TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioTarget) ||
				TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioPlatform))
			{
				if (UStaticMeshComponent* StaticMeshComp = OriginalTarget->FindComponentByClass<UStaticMeshComponent>())
				{
					OutEndLocation = StaticMeshComp->GetComponentLocation();
					return true;
				}
			}
		}

		// fallback 1 : Character mesh
		if (ACharacter* TargetCharacter = Cast<ACharacter>(OriginalTarget))
		{
			if (USkeletalMeshComponent* CharacterMesh = TargetCharacter->GetMesh())
			{
				OutEndLocation = CharacterMesh->GetComponentLocation();
				return true;
			}
		}

		// fallback 2 : StaticMesh
		if (UStaticMeshComponent* StaticMeshComp = OriginalTarget->FindComponentByClass<UStaticMeshComponent>())
		{
			OutEndLocation = StaticMeshComp->GetComponentLocation();
			return true;
		}

		// fallback 3 : Physics sim primitive
		TArray<UPrimitiveComponent*> PrimitiveComps;
		OriginalTarget->GetComponents<UPrimitiveComponent>(PrimitiveComps);

		for (UPrimitiveComponent* PrimComp : PrimitiveComps)
		{
			if (PrimComp && PrimComp->IsSimulatingPhysics())
			{
				OutEndLocation = PrimComp->GetComponentLocation();
				return true;
			}
		}

		// final fallback
		OutEndLocation = OriginalTarget->GetActorLocation();
		return true;
	}

	// =========================
	// 2) OriginalTarget이 없을 때만 이동 대상 fallback
	// =========================
	if (IsValid(TargetToMove))
	{
		if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetToMove))
		{
			if (USkeletalMeshComponent* CharacterMesh = TargetCharacter->GetMesh())
			{
				OutEndLocation = CharacterMesh->GetComponentLocation();
				return true;
			}
		}

		if (UStaticMeshComponent* StaticMeshComp = TargetToMove->FindComponentByClass<UStaticMeshComponent>())
		{
			OutEndLocation = StaticMeshComp->GetComponentLocation();
			return true;
		}

		TArray<UPrimitiveComponent*> PrimitiveComps;
		TargetToMove->GetComponents<UPrimitiveComponent>(PrimitiveComps);

		for (UPrimitiveComponent* PrimComp : PrimitiveComps)
		{
			if (PrimComp && PrimComp->IsSimulatingPhysics())
			{
				OutEndLocation = PrimComp->GetComponentLocation();
				return true;
			}
		}

		OutEndLocation = TargetToMove->GetActorLocation();
		return true;
	}

	return false;
}

bool UGA_Spell_Accio::FireAccio()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return false;
	}

	FGameplayTagContainer TargetTags;
	FVector AimPoint;
	AActor* AcquiredTarget = nullptr;

	const bool bHasTarget = TryConsumeLockedTarget(AcquiredTarget, TargetTags, AimPoint);

	if (!IsValid(AcquiredTarget))
	{
		UWorld* World = GetWorld();
		if (World)
		{
			FVector StartLoc = Avatar->GetActorLocation();
			FVector TargetLoc = AimPoint.IsNearlyZero()
				                    ? StartLoc + (Avatar->GetActorForwardVector() * GetCastRange())
				                    : AimPoint;

			const float SweepRadius = 50.f;
			const FCollisionShape SphereShape = FCollisionShape::MakeSphere(SweepRadius);
			FCollisionQueryParams Params(SCENE_QUERY_STAT(AccioTrace), false, Avatar);

			TArray<FHitResult> HitResults;
			const bool bHit = World->SweepMultiByChannel(
				HitResults,
				StartLoc,
				TargetLoc,
				FQuat::Identity,
				ECC_Visibility,
				SphereShape,
				Params
			);

			if (bHit)
			{
				for (const FHitResult& Hit : HitResults)
				{
					AActor* HitActor = Hit.GetActor();
					if (!IsValid(HitActor) || HitActor == Avatar)
					{
						continue;
					}

					UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(
						HitActor);
					const bool bIsTargetNode = TargetASC && TargetASC->HasMatchingGameplayTag(
						HOGGameplayTags::Interactable_AccioTarget);

					bool bIsSimulatingPhysics = false;
					TArray<UPrimitiveComponent*> PrimitiveComps;
					HitActor->GetComponents<UPrimitiveComponent>(PrimitiveComps);

					for (UPrimitiveComponent* PrimComp : PrimitiveComps)
					{
						if (PrimComp && PrimComp->IsSimulatingPhysics())
						{
							bIsSimulatingPhysics = true;
							break;
						}
					}

					const bool bIsMovable = HitActor->IsA<ACharacter>() || bIsSimulatingPhysics;

					if (bIsTargetNode || bIsMovable)
					{
						AcquiredTarget = HitActor;
						break;
					}
				}
			}
		}
	}

	if (!IsValid(AcquiredTarget))
	{
		return false;
	}

	OriginalTarget = AcquiredTarget;
	TargetToMove = nullptr;
	PullDestination = nullptr;
	bIsPullingInteractable = false;
	CurrentPullSpeed = DefaultPullSpeed;

	ACharacter* AvatarChar = Cast<ACharacter>(Avatar);
	UPrimitiveComponent* MovementBase = AvatarChar ? AvatarChar->GetCharacterMovement()->GetMovementBase() : nullptr;
	AActor* CurrentFloorActor = MovementBase ? MovementBase->GetOwner() : nullptr;

	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AcquiredTarget);
	const bool bIsHitTarget = TargetASC && TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioTarget);

	UAbilitySystemComponent* FloorASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(CurrentFloorActor);
	const bool bIsStandingOnPlatform = FloorASC && FloorASC->HasMatchingGameplayTag(
		HOGGameplayTags::Interactable_AccioPlatform);

	bool bCanBeMovedByAccioTag = false;

	if (TargetASC)
	{
		if (TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioTarget) ||
			TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioPlatform) ||
			TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Team_Enemy) ||
			TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Unit.Enemy"))))
		{
			bCanBeMovedByAccioTag = true;
		}
	}

	if (!bCanBeMovedByAccioTag)
	{
		return false;
	}

	if (TargetASC)
	{
		if (TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioPlatform) ||
			TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Interactable_AccioTarget))
		{
			bIsPullingInteractable = true;
			CurrentPullSpeed = InteractablePullSpeed;
		}
		else if (TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("Unit.Enemy"))) ||
			TargetASC->HasMatchingGameplayTag(HOGGameplayTags::Team_Enemy))
		{
			bIsPullingInteractable = false;
			CurrentPullSpeed = EnemyPullSpeed;
		}
	}

	if (bIsHitTarget)
	{
		if (bIsStandingOnPlatform)
		{
			TargetToMove = CurrentFloorActor;
			PullDestination = AcquiredTarget;
		}
		else
		{
			return false;
		}
	}
	else
	{
		TargetToMove = AcquiredTarget;
		PullDestination = Avatar;
	}

	if (PullSound && IsValid(TargetToMove) && TargetToMove->GetRootComponent())
	{
		PullAudioComponent = UGameplayStatics::SpawnSoundAttached(PullSound, TargetToMove->GetRootComponent());
	}

	if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetToMove))
	{
		TargetCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	}
	else
	{
		TArray<UPrimitiveComponent*> PrimitiveComps;
		TargetToMove->GetComponents<UPrimitiveComponent>(PrimitiveComps);

		for (UPrimitiveComponent* MovePrimComp : PrimitiveComps)
		{
			if (MovePrimComp && MovePrimComp->IsSimulatingPhysics())
			{
				UAbilitySystemComponent* MoveASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(
					TargetToMove);
				const bool bIsPlatform = MoveASC && MoveASC->HasMatchingGameplayTag(
					HOGGameplayTags::Interactable_AccioPlatform);

				if (!bIsPlatform)
				{
					MovePrimComp->SetEnableGravity(false);
				}
				break;
			}
		}
	}

	GetWorld()->GetTimerManager().SetTimer(PullTimerHandle, this, &UGA_Spell_Accio::UpdatePulling, 0.02f, true);
	return true;
}

void UGA_Spell_Accio::UpdatePulling()
{
	UpdatePersistentBeamVFX();

	if (!IsValid(TargetToMove) || !IsValid(PullDestination))
	{
		GetWorld()->GetTimerManager().ClearTimer(PullTimerHandle);
		BeginMontageEndTransition(true, false);
		return;
	}

	FVector DestLoc = PullDestination->GetActorLocation();
	if (!PullDestination->IsA<ACharacter>())
	{
		TArray<UPrimitiveComponent*> DestPrimComps;
		PullDestination->GetComponents<UPrimitiveComponent>(DestPrimComps);

		for (UPrimitiveComponent* PrimComp : DestPrimComps)
		{
			if (PrimComp && (PrimComp->IsA<UStaticMeshComponent>() || PrimComp->IsSimulatingPhysics()))
			{
				DestLoc = PrimComp->GetComponentLocation();
				break;
			}
		}
	}

	FVector MoveLoc = TargetToMove->GetActorLocation();
	UPrimitiveComponent* ActualMoveComp = nullptr;

	if (!TargetToMove->IsA<ACharacter>())
	{
		TArray<UPrimitiveComponent*> MovePrimComps;
		TargetToMove->GetComponents<UPrimitiveComponent>(MovePrimComps);

		for (UPrimitiveComponent* MovePrimComp : MovePrimComps)
		{
			if (MovePrimComp && MovePrimComp->IsSimulatingPhysics())
			{
				MoveLoc = MovePrimComp->GetComponentLocation();
				ActualMoveComp = MovePrimComp;
				break;
			}
		}
	}

	const float Distance = FVector::Dist2D(DestLoc, MoveLoc);

	if (Distance <= StopDistance)
	{
		GetWorld()->GetTimerManager().ClearTimer(PullTimerHandle);

		if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetToMove))
		{
			TargetCharacter->GetCharacterMovement()->Velocity = FVector::ZeroVector;
		}
		else if (ActualMoveComp)
		{
			ActualMoveComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
		}

		BeginMontageEndTransition(true, false);
		return;
	}

	FVector Direction = (DestLoc - MoveLoc);
	Direction.Z = 0.f;
	const FVector PullDirection = Direction.GetSafeNormal();

	if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetToMove))
	{
		TargetCharacter->GetCharacterMovement()->Velocity = PullDirection * CurrentPullSpeed;
	}
	else if (ActualMoveComp)
	{
		ActualMoveComp->SetPhysicsLinearVelocity(PullDirection * CurrentPullSpeed);
	}
}

void UGA_Spell_Accio::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CastMontage)
	{
		return;
	}

	if (bInterrupted)
	{
		if (bIgnoreNextCastMontageInterrupted)
		{
			bIgnoreNextCastMontageInterrupted = false;
			return;
		}

		FinishAccioAbilityEnd(true, true);
		return;
	}

	if (!bCastNotifyHandled)
	{
		HandleCastNotify();
		return;
	}

	if (bPendingMontageEndTransition)
	{
		const bool bReplicate = bPendingEndAbilityReplicate;
		const bool bWasCancelled = bPendingEndAbilityWasCancelled;

		bPendingMontageEndTransition = false;
		bPendingEndAbilityReplicate = false;
		bPendingEndAbilityWasCancelled = false;

		FinishAccioAbilityEnd(bReplicate, bWasCancelled);
	}
}

void UGA_Spell_Accio::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PullTimerHandle);
	}

	ClearPersistentBeamVFX();

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (Character && Character->GetMesh())
	{
		if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
		{
			if (HoldLoopMontage)
			{
				AnimInstance->Montage_Stop(0.1f, HoldLoopMontage);
			}
		}
	}

	if (PullAudioComponent)
	{
		PullAudioComponent->Stop();
		PullAudioComponent = nullptr;
	}

	if (IsValid(TargetToMove))
	{
		if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetToMove))
		{
			TargetCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
		}
		else
		{
			TArray<UPrimitiveComponent*> PrimitiveComps;
			TargetToMove->GetComponents<UPrimitiveComponent>(PrimitiveComps);

			for (UPrimitiveComponent* MovePrimComp : PrimitiveComps)
			{
				if (MovePrimComp && MovePrimComp->IsSimulatingPhysics())
				{
					UAbilitySystemComponent* MoveASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(
						TargetToMove);
					const bool bIsPlatform = MoveASC && MoveASC->HasMatchingGameplayTag(
						HOGGameplayTags::Interactable_AccioPlatform);

					if (!bIsPlatform)
					{
						MovePrimComp->SetEnableGravity(true);
					}

					MovePrimComp->SetPhysicsLinearVelocity(FVector::ZeroVector);
					break;
				}
			}
		}
	}

	TargetToMove = nullptr;
	PullDestination = nullptr;
	OriginalTarget = nullptr;
	bCastNotifyHandled = false;
	bPendingMontageEndTransition = false;
	bPendingEndAbilityReplicate = false;
	bPendingEndAbilityWasCancelled = false;
	bIsPullingInteractable = false;
	bIgnoreNextCastMontageInterrupted = false;
	CurrentPullSpeed = 0.f;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
