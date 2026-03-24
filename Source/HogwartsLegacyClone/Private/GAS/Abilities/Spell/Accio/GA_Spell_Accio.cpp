#include "GAS/Abilities/Spell/Accio/GA_Spell_Accio.h"

#include "HOGDebugHelper.h"
#include "Core/HOG_GameplayTags.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

#include "Components/AudioComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

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
		Debug::Print(TEXT("[Accio] CheckCooldown Failed"));
		FinishAccioAbilityEnd(true, false);
		return;
	}

	if (TryBeginPreCastFacing(Handle, ActorInfo, ActivationInfo, TriggerEventData))
	{
		Debug::Print(TEXT("[Accio] Waiting PreCastFacing"));
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
	Debug::Print(TEXT("[Accio] OnPreCastFacingFinished"));
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
		Debug::Print(TEXT("[Accio] CommitAbility Failed"));
		FinishAccioAbilityEnd(true, true);
		return;
	}

	Debug::Print(TEXT("[Accio] ExecuteAccioCast Committed"));

	ACharacter* Character = Cast<ACharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);

	if (CastVoiceSound && Character)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastVoiceSound, Character->GetActorLocation());
	}

	if (CastSound && Character)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastSound, Character->GetActorLocation());
	}

	RegisterCastNotifyToOwner();
	Debug::Print(TEXT("[Accio] RegisterCastNotifyToOwner"));

	if (Character && Character->GetMesh() && CastMontage)
	{
		if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
		{
			const float Duration = AnimInstance->Montage_Play(CastMontage, 1.0f);
			if (Duration > 0.f)
			{
				Debug::Print(TEXT("[Accio] CastMontage Play Success"));

				FOnMontageEnded EndDelegate;
				EndDelegate.BindUObject(this, &UGA_Spell_Accio::OnMontageEnded);
				AnimInstance->Montage_SetEndDelegate(EndDelegate, CastMontage);

				const bool bJumpedStart = TryJumpMontageToSection(StartSectionName);
				Debug::Print(FString::Printf(TEXT("[Accio] Jump Start Result = %d"), bJumpedStart ? 1 : 0));
				return;
			}
		}
	}

	Debug::Print(TEXT("[Accio] Montage Fallback -> HandleCastNotify"));
	HandleCastNotify();
}

void UGA_Spell_Accio::InputPressed(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	Debug::Print(TEXT("[Accio] InputPressed"));

	if (IsValid(TargetToMove))
	{
		Debug::Print(TEXT("[Accio] InputPressed -> BeginMontageEndTransition"));
		BeginMontageEndTransition(true, false);
	}
}

void UGA_Spell_Accio::HandleCastNotify()
{
	Debug::Print(TEXT("[Accio] HandleCastNotify Called"));

	if (bCastNotifyHandled)
	{
		Debug::Print(TEXT("[Accio] HandleCastNotify Ignored AlreadyHandled"));
		return;
	}

	bCastNotifyHandled = true;

	if (!FireAccio())
	{
		Debug::Print(TEXT("[Accio] FireAccio Failed"));
		FinishAccioAbilityEnd(true, false);
		return;
	}

	Debug::Print(TEXT("[Accio] FireAccio Success"));

	const bool bBeamSpawned = SpawnPersistentBeamVFX();
	Debug::Print(FString::Printf(TEXT("[Accio] SpawnPersistentBeamVFX = %d"), bBeamSpawned ? 1 : 0));

	const bool bJumpedHold = TryJumpMontageToSection(HoldSectionName);
	Debug::Print(FString::Printf(TEXT("[Accio] Jump Hold Result = %d"), bJumpedHold ? 1 : 0));
}

bool UGA_Spell_Accio::TryJumpMontageToSection(FName SectionName) const
{
	if (SectionName.IsNone() || !CastMontage)
	{
		Debug::Print(TEXT("[Accio] TryJumpMontageToSection Failed | Section None or CastMontage Null"));
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!Character || !Character->GetMesh())
	{
		Debug::Print(TEXT("[Accio] TryJumpMontageToSection Failed | Character/Mesh Null"));
		return false;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		Debug::Print(TEXT("[Accio] TryJumpMontageToSection Failed | AnimInstance Null"));
		return false;
	}

	Debug::Print(FString::Printf(
		TEXT("[Accio] Montage_IsPlaying = %d | Try Section = %s"),
		AnimInstance->Montage_IsPlaying(CastMontage) ? 1 : 0,
		*SectionName.ToString()
	));

	if (!AnimInstance->Montage_IsPlaying(CastMontage))
	{
		return false;
	}

	AnimInstance->Montage_JumpToSection(SectionName, CastMontage);

	Debug::Print(FString::Printf(TEXT("[Accio] JumpToSection Success : %s"), *SectionName.ToString()));
	return true;
}

void UGA_Spell_Accio::BeginMontageEndTransition(bool bReplicateEndAbility, bool bWasCancelled)
{
	Debug::Print(TEXT("[Accio] BeginMontageEndTransition Called"));

	if (bPendingMontageEndTransition)
	{
		Debug::Print(TEXT("[Accio] BeginMontageEndTransition Ignored AlreadyPending"));
		return;
	}

	bPendingMontageEndTransition = true;
	bPendingEndAbilityReplicate = bReplicateEndAbility;
	bPendingEndAbilityWasCancelled = bWasCancelled;

	const bool bJumped = TryJumpMontageToSection(EndSectionName);
	Debug::Print(FString::Printf(TEXT("[Accio] Jump End Result = %d"), bJumped ? 1 : 0));

	if (!bJumped)
	{
		Debug::Print(TEXT("[Accio] End Section Jump Failed -> FinishAccioAbilityEnd"));
		FinishAccioAbilityEnd(bReplicateEndAbility, bWasCancelled);
	}
}

void UGA_Spell_Accio::FinishAccioAbilityEnd(bool bReplicateEndAbility, bool bWasCancelled)
{
	Debug::Print(FString::Printf(
		TEXT("[Accio] FinishAccioAbilityEnd | Rep=%d | Cancel=%d"),
		bReplicateEndAbility ? 1 : 0,
		bWasCancelled ? 1 : 0
	));

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
		Debug::Print(TEXT("[Accio] SpawnPersistentBeamVFX Failed | AccioVFX Null"));
		return false;
	}

	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;

	if (!GetCurrentBeamStartLocation(StartLocation))
	{
		Debug::Print(TEXT("[Accio] SpawnPersistentBeamVFX Failed | StartLocation Invalid"));
		return false;
	}

	if (!GetCurrentBeamEndLocation(EndLocation))
	{
		Debug::Print(TEXT("[Accio] SpawnPersistentBeamVFX Failed | EndLocation Invalid"));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		Debug::Print(TEXT("[Accio] SpawnPersistentBeamVFX Failed | World Null"));
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
		Debug::Print(TEXT("[Accio] SpawnPersistentBeamVFX Failed | Niagara Spawn Null"));
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
		Debug::Print(TEXT("[Accio] UpdatePersistentBeamVFX Invalid Start/End -> Clear"));
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
	ACharacter* Character = Cast<ACharacter>(Avatar);
	if (!Character)
	{
		return false;
	}

	USkeletalMeshComponent* MeshComp = Character->GetMesh();
	if (!MeshComp)
	{
		return false;
	}

	if (BeamStartSocketName != NAME_None && MeshComp->DoesSocketExist(BeamStartSocketName))
	{
		OutStartLocation = MeshComp->GetSocketLocation(BeamStartSocketName);
	}
	else
	{
		OutStartLocation = Character->GetActorLocation();
	}

	return true;
}

bool UGA_Spell_Accio::GetCurrentBeamEndLocation(FVector& OutEndLocation) const
{
	OutEndLocation = FVector::ZeroVector;

	if (IsValid(TargetToMove))
	{
		if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetToMove))
		{
			OutEndLocation = TargetCharacter->GetActorLocation();
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

	if (IsValid(OriginalTarget))
	{
		OutEndLocation = OriginalTarget->GetActorLocation();
		return true;
	}

	return false;
}

bool UGA_Spell_Accio::FireAccio()
{
	Debug::Print(TEXT("[Accio] FireAccio Enter"));

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		Debug::Print(TEXT("[Accio] FireAccio Failed | Avatar Null"));
		return false;
	}

	FGameplayTagContainer TargetTags;
	FVector AimPoint;
	AActor* AcquiredTarget = nullptr;

	const bool bHasTarget = TryConsumeLockedTarget(AcquiredTarget, TargetTags, AimPoint);
	Debug::Print(FString::Printf(TEXT("[Accio] TryConsumeLockedTarget HasTarget=%d"), bHasTarget ? 1 : 0));

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

			DrawDebugLine(World, StartLoc, TargetLoc, FColor::Green, false, 2.0f, 0, 2.0f);
			DrawDebugSphere(World, StartLoc, SweepRadius, 16, FColor::Green, false, 2.0f);
			DrawDebugSphere(World, TargetLoc, SweepRadius, 16, FColor::Green, false, 2.0f);

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
					DrawDebugPoint(World, Hit.ImpactPoint, 15.f, FColor::Red, false, 2.0f);

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
						DrawDebugBox(World, AcquiredTarget->GetActorLocation(), FVector(60.f), FColor::Cyan, false,
						             2.0f);
						break;
					}
				}
			}
		}
	}

	if (!IsValid(AcquiredTarget))
	{
		Debug::Print(TEXT("[Accio] FireAccio Failed | No AcquiredTarget"));
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
		Debug::Print(TEXT("[Accio] FireAccio Failed | Target Not Movable By Accio Tag"));
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
			Debug::Print(TEXT("[Accio] FireAccio Failed | Need Platform First"));
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
	Debug::Print(TEXT("[Accio] FireAccio Success | PullTimer Started"));
	return true;
}

void UGA_Spell_Accio::UpdatePulling()
{
	UpdatePersistentBeamVFX();

	if (!IsValid(TargetToMove) || !IsValid(PullDestination))
	{
		Debug::Print(TEXT("[Accio] UpdatePulling Invalid Target/Destination"));
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
	//Debug::Print(FString::Printf(TEXT("[Accio] Distance = %.2f | StopDistance = %.2f"), Distance, StopDistance));

	if (Distance <= StopDistance)
	{
		Debug::Print(TEXT("[Accio] Reached StopDistance -> BeginMontageEndTransition"));

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
	Debug::Print(TEXT("[Accio] OnMontageEnded Called"));

	if (bInterrupted)
	{
		Debug::Print(TEXT("[Accio] OnMontageEnded Interrupted"));
		FinishAccioAbilityEnd(true, true);
		return;
	}

	if (!bCastNotifyHandled)
	{
		Debug::Print(TEXT("[Accio] OnMontageEnded Fallback -> HandleCastNotify"));
		HandleCastNotify();
		return;
	}

	if (bPendingMontageEndTransition)
	{
		Debug::Print(TEXT("[Accio] OnMontageEnded PendingEndTransition -> FinishAccioAbilityEnd"));

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
	Debug::Print(TEXT("[Accio] EndAbility Enter"));

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PullTimerHandle);
	}

	ClearPersistentBeamVFX();

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
	CurrentPullSpeed = 0.f;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
