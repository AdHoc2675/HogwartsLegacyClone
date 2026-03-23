#include "GAS/Abilities/GA_SpellBase.h"

#include "HOGDebugHelper.h"
#include "Data/DA_SpellDefinition.h"
#include "GameFramework/HOG_GameInstance.h"
#include "GameFramework/HOG_PlayerState.h"
#include "Component/SpellComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Character/Player/PlayerCharacterBase.h"
#include "Component/LockOnComponent.h"
#include "TimerManager.h"
#include "CollisionQueryParams.h"
#include "Core/HOG_Struct.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

UGA_SpellBase::UGA_SpellBase()
{
	// 기본적으로 SpellBase 자체는 “설정(Definition 조회/검증)”만 담당한다.
	// 실제 공격/퍼즐 로직은 파생 Ability에서 구현.
	// 따라서 별도의 정책 변경은 하지 않고 GA_Base의 기본 정책을 사용한다.

	bWarnIfDefinitionMissing = true;
	bRequireFacingBeforeCast = true;
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

	/*
	Debug::Print(Msg);
	*/
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

	AActor* LockedTarget = LockOn->GetCurrentTarget();
	if (!IsValid(LockedTarget))
	{
		BuildFallbackAimPoint(OutAimPoint, Def->CastRange);
		return false;
	}

	if (!DoesTargetMeetRequirements(LockedTarget))
	{
		BuildFallbackAimPoint(OutAimPoint, Def->CastRange);
		return false;
	}

	OutTarget = LockedTarget;
	OutAimPoint = LockedTarget->GetActorLocation();

	if (LockedTarget->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
	{
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(LockedTarget);
		if (ASI)
		{
			if (UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent())
			{
				TargetASC->GetOwnedGameplayTags(OutTargetTags);
			}
		}
	}

	return true;
}

bool UGA_SpellBase::BuildFallbackAimPoint(FVector& OutAimPoint, float RangeOverride) const
{
	OutAimPoint = FVector::ZeroVector;

	if (!CurrentActorInfo)
	{
		return false;
	}

	AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
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

	const FVector CameraLoc = PC->PlayerCameraManager->GetCameraLocation();
	const FVector CameraForward = PC->PlayerCameraManager->GetActorForwardVector();

	float TraceDistance = RangeOverride;
	if (TraceDistance <= 0.f)
	{
		TraceDistance = GetCastRange();
	}

	// 근거리 주문이어도 카메라-캐릭터 사이의 짧은 점을 잡지 않도록 최소값 보장
	TraceDistance = FMath::Max(TraceDistance, 5000.f);

	const FVector TraceStart = CameraLoc;
	const FVector TraceEnd = TraceStart + (CameraForward * TraceDistance);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SpellBaseFallbackAim), false, Avatar);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		TraceStart,
		TraceEnd,
		ECC_Visibility,
		Params
	);

	OutAimPoint = bHit ? Hit.ImpactPoint : TraceEnd;
	return true;
}

bool UGA_SpellBase::GetCachedPreCastFacingTargetLocation(FVector& OutTargetLocation) const
{
	if (!bHasCachedPreCastFacingTargetLocation)
	{
		return false;
	}

	OutTargetLocation = CachedPreCastFacingTargetLocation;
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
}

void UGA_SpellBase::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled
)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PreCastFacingTimerHandle);
	}

	bWaitingForPreCastFacing = false;
	PreCastFacingElapsed = 0.f;
	bHasCachedPreCastFacingTargetLocation = false;
	CachedPreCastFacingTargetLocation = FVector::ZeroVector;
	CachedFacingAbilityForSafety = nullptr;

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

bool UGA_SpellBase::TryBuildPreCastFacingTargetLocation(FVector& OutTargetLocation) const
{
	AActor* LockedTarget = nullptr;
	FGameplayTagContainer LockedTargetTags;
	FVector AimPoint = FVector::ZeroVector;

	if (TryConsumeLockedTarget(LockedTarget, LockedTargetTags, AimPoint))
	{
		OutTargetLocation = AimPoint.IsNearlyZero()
			? LockedTarget->GetActorLocation()
			: AimPoint;

		return true;
	}

	if (BuildFallbackAimPoint(AimPoint))
	{
		OutTargetLocation = AimPoint;
		return true;
	}

	return false;
}

bool UGA_SpellBase::TryBeginPreCastFacing(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	if (!bRequireFacingBeforeCast)
	{
		return false;
	}

	if (bWaitingForPreCastFacing)
	{
		return true;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(Avatar);
	if (!PlayerCharacter)
	{
		return false;
	}

	FVector TargetLocation = FVector::ZeroVector;
	if (!TryBuildPreCastFacingTargetLocation(TargetLocation))
	{
		return false;
	}

	CachedPreCastFacingTargetLocation = TargetLocation;
	bHasCachedPreCastFacingTargetLocation = true;

	PlayerCharacter->BeginForcedFacingToLocation(TargetLocation);

	CachedFacingHandle = Handle;
	CachedFacingActivationInfo = ActivationInfo;
	CachedFacingAbilityForSafety = this;

	bWaitingForPreCastFacing = true;
	PreCastFacingElapsed = 0.f;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PreCastFacingTimerHandle,
			this,
			&UGA_SpellBase::TickPreCastFacing,
			PreCastFacingPollInterval,
			true
		);
	}

	return ShouldDeferCastUntilFacingFinished();
}

void UGA_SpellBase::TickPreCastFacing()
{
	if (!bWaitingForPreCastFacing)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(PreCastFacingTimerHandle);
		}
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(Avatar);
	if (!PlayerCharacter)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(PreCastFacingTimerHandle);
		}

		bWaitingForPreCastFacing = false;
		return;
	}

	PreCastFacingElapsed += PreCastFacingPollInterval;

	if (PlayerCharacter->IsForcedFacingFinished() || PreCastFacingElapsed >= PreCastFacingTimeout)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(PreCastFacingTimerHandle);
		}

		PlayerCharacter->StopForcedFacing();
		bWaitingForPreCastFacing = false;

		OnPreCastFacingFinished(
			CachedFacingHandle,
			CurrentActorInfo,
			CachedFacingActivationInfo,
			nullptr
		);
	}
}

void UGA_SpellBase::OnPreCastFacingFinished(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData
)
{
	// 파생 ability 에서 override
}

bool UGA_SpellBase::ShouldDeferCastUntilFacingFinished() const
{
	return true;
}

void UGA_SpellBase::QueueLineTraceSpellVFX(
	UNiagaraSystem* InVFX,
	const FVector& InTargetLocation,
	FName InStartSocketName,
	FName InBeamStartParam,
	FName InBeamEndParam,
	FName InBeamLengthParam
)
{
	if (!InVFX)
	{
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(Avatar);
	if (!PlayerCharacter)
	{
		return;
	}

	FQueuedSpellVFXData VFXData;
	VFXData.bPending = true;
	VFXData.NiagaraSystem = InVFX;
	VFXData.TargetLocation = InTargetLocation;
	VFXData.StartSocketName = InStartSocketName;
	VFXData.BeamStartParameterName = InBeamStartParam;
	VFXData.BeamEndParameterName = InBeamEndParam;
	VFXData.BeamLengthParameterName = InBeamLengthParam;

	PlayerCharacter->QueueSpellVFX(VFXData);
}

void UGA_SpellBase::ClearQueuedLineTraceSpellVFX()
{
	QueuedLineTraceBeamVFX = nullptr;
	QueuedLineTraceBeamTargetLocation = FVector::ZeroVector;
	QueuedBeamStartSocketName = NAME_None;
	QueuedBeamStartParamName = TEXT("BeamStart");
	QueuedBeamEndParamName = TEXT("BeamEnd");
	QueuedBeamLengthParamName = TEXT("BeamLength");
}

bool UGA_SpellBase::SpawnQueuedLineTraceSpellVFX()
{
	if (!QueuedLineTraceBeamVFX)
	{
		return false;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	ACharacter* Character = Cast<ACharacter>(Avatar);
	if (!Character)
	{
		ClearQueuedLineTraceSpellVFX();
		return false;
	}

	USkeletalMeshComponent* MeshComp = Character->GetMesh();
	if (!MeshComp)
	{
		ClearQueuedLineTraceSpellVFX();
		return false;
	}

	FVector StartLocation = Character->GetActorLocation();

	if (QueuedBeamStartSocketName != NAME_None &&
		MeshComp->DoesSocketExist(QueuedBeamStartSocketName))
	{
		StartLocation = MeshComp->GetSocketLocation(QueuedBeamStartSocketName);
	}

	const FRotator SpawnRotation =
		(QueuedLineTraceBeamTargetLocation - StartLocation).Rotation();

	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		Character->GetWorld(),
		QueuedLineTraceBeamVFX,
		StartLocation,
		SpawnRotation,
		FVector(1.f),
		true,
		true,
		ENCPoolMethod::None,
		true
	);

	if (NiagaraComp)
	{
		NiagaraComp->SetVectorParameter(QueuedBeamStartParamName, StartLocation);
		NiagaraComp->SetVectorParameter(QueuedBeamEndParamName, QueuedLineTraceBeamTargetLocation);
		NiagaraComp->SetFloatParameter(
			QueuedBeamLengthParamName,
			FVector::Distance(StartLocation, QueuedLineTraceBeamTargetLocation)
		);
	}

	ClearQueuedLineTraceSpellVFX();
	return NiagaraComp != nullptr;
}

void UGA_SpellBase::RegisterCastNotifyToOwner()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(Avatar);
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->QueueCastNotifyAbility(this);
}

void UGA_SpellBase::HandleCastNotify()
{
	// 기본 SpellBase는 공용 베이스.
	// 실제 판정/추가 처리(예: Stupefy 데미지 적용)는 파생 클래스에서 override 한다.
}