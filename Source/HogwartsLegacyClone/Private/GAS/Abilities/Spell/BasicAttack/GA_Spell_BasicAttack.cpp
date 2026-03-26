#include "GAS/Abilities/Spell/BasicAttack/GA_Spell_BasicAttack.h"

#include "Data/DA_SpellDefinition.h"
#include "Character/Player/PlayerCharacterBase.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "AbilitySystemComponent.h"
#include "HOGDebugHelper.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h" 

#include "GAS/Abilities/Spell/BasicAttack/BasicAttackActor.h"

UGA_Spell_BasicAttack::UGA_Spell_BasicAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ComboIndex = 0;
	bComboInProgress = false;
	bNextComboQueued = false;
	CurrentComboStep = 0;
	LastComboQueuedTime = -1.f;
	bBranchConsumed = false;
	CurrentPlayingMontage = nullptr;
	bAdvancingComboFromBranch = false;
	bProjectileFiredThisStep = false;
}

void UGA_Spell_BasicAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	FGameplayTagContainer RelevantTags;
	if (!CheckCooldown(Handle, ActorInfo, &RelevantTags))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (TryBeginPreCastFacing(Handle, ActorInfo, ActivationInfo, TriggerEventData))
	{
		return;
	}

	ExecuteBasicAttackCast(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Spell_BasicAttack::OnPreCastFacingFinished(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ExecuteBasicAttackCast(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_Spell_BasicAttack::ExecuteBasicAttackCast(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 첫 입력: 콤보 시작
	bComboInProgress = true;
	bNextComboQueued = false;
	CurrentComboStep = 0;
	bBranchConsumed = false;
	bAdvancingComboFromBranch = false;
	CurrentPlayingMontage = nullptr;
	bProjectileFiredThisStep = false;

	PlayComboMontageOrFire(ActorInfo);
}

void UGA_Spell_BasicAttack::InputPressed(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	APlayerCharacterBase* PlayerCharacter = ActorInfo
		? Cast<APlayerCharacterBase>(ActorInfo->AvatarActor.Get())
		: nullptr;

	if (!bComboInProgress)
	{
		return;
	}

	if (!PlayerCharacter)
	{
		return;
	}

	if (PlayerCharacter->CanQueueNextCombo())
	{
		bNextComboQueued = true;

		if (UWorld* World = GetWorld())
		{
			LastComboQueuedTime = World->GetTimeSeconds();
		}
	}
}

void UGA_Spell_BasicAttack::PlayComboMontageOrFire(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!TryPlayCurrentComboMontage(ActorInfo))
	{
		FireHitScan(ActorInfo);
		ResetComboState();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 투사체 생성은 AnimNotify(UAN_BasicAttackFire)에서만 처리
	//FireHitScan(ActorInfo);
	//SpawnBasicAttackActor();
}

bool UGA_Spell_BasicAttack::TryPlayCurrentComboMontage(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!ActorInfo)
	{
		return false;
	}

	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		return false;
	}

	UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return false;
	}

	if (!ComboMontages.IsValidIndex(CurrentComboStep))
	{
		return false;
	}

	UAnimMontage* MontageToPlay = ComboMontages[CurrentComboStep];
	if (!MontageToPlay)
	{
		return false;
	}

	const float PlayResult = Character->PlayAnimMontage(MontageToPlay);
	if (PlayResult <= 0.f)
	{
		return false;
	}

	CurrentPlayingMontage = MontageToPlay;
	bComboInProgress = true;
	bNextComboQueued = false;
	bBranchConsumed = false;
	bAdvancingComboFromBranch = false;
	bProjectileFiredThisStep = false;

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UGA_Spell_BasicAttack::OnMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, MontageToPlay);

	return true;
}

void UGA_Spell_BasicAttack::TryAdvanceComboFromBranchPoint()
{
	if (!bComboInProgress)
	{
		return;
	}

	if (bBranchConsumed)
	{
		return;
	}

	if (!IsComboInputBufferedValid())
	{
		return;
	}

	const int32 NextStep = CurrentComboStep + 1;
	const bool bHasNextStep = ComboMontages.IsValidIndex(NextStep);

	if (!bHasNextStep)
	{
		return;
	}

	bBranchConsumed = true;
	bNextComboQueued = false;
	LastComboQueuedTime = -1.f;
	bAdvancingComboFromBranch = true;
	CurrentComboStep = NextStep;

	if (!TryPlayCurrentComboMontage(CurrentActorInfo))
	{
		ResetComboState();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 투사체 생성은 AnimNotify(UAN_BasicAttackFire)에서만 처리
	//FireHitScan(CurrentActorInfo);
	//SpawnBasicAttackActor();
}

void UGA_Spell_BasicAttack::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage != CurrentPlayingMontage)
	{
		return;
	}

	if (bInterrupted && bAdvancingComboFromBranch)
	{
		return;
	}

	if (bInterrupted)
	{
		ResetComboState();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	const int32 NextStep = CurrentComboStep + 1;
	const bool bHasNextStep = ComboMontages.IsValidIndex(NextStep);

	if (IsComboInputBufferedValid() && bHasNextStep)
	{
		CurrentComboStep = NextStep;
		bNextComboQueued = false;
		LastComboQueuedTime = -1.f;

		if (!TryPlayCurrentComboMontage(CurrentActorInfo))
		{
			ResetComboState();
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			return;
		}

		// 투사체 생성은 AnimNotify(UAN_BasicAttackFire)에서만 처리
		//FireHitScan(CurrentActorInfo);
		//SpawnBasicAttackActor();
		return;
	}

	ResetComboState();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Spell_BasicAttack::ResetComboState()
{
	bComboInProgress = false;
	bNextComboQueued = false;
	CurrentComboStep = 0;
	ComboIndex = 0;
	bBranchConsumed = false;
	CurrentPlayingMontage = nullptr;
	bAdvancingComboFromBranch = false;
	LastComboQueuedTime = -1.f;
	bProjectileFiredThisStep = false;
}

void UGA_Spell_BasicAttack::SpawnBasicAttackActor()
{
	if (bProjectileFiredThisStep)
	{
		return;
	}

	if (!BasicAttackActorClass)
	{
		return;
	}

	ACharacter* AvatarCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(AvatarCharacter))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!AvatarCharacter->GetMesh())
	{
		return;
	}

	/* =========================
	   Spawn 위치 계산
	========================= */

	const FName WandSocketName(TEXT("RightHandWandSocket"));

	const FVector SpawnLocation =
		AvatarCharacter->GetMesh()->GetSocketLocation(WandSocketName);

	FVector ShootDirection = AvatarCharacter->GetActorForwardVector();

	/* =========================
	   LockOn 타겟 획득
	========================= */

	AActor* TargetActor = nullptr;
	FGameplayTagContainer TargetTags;
	FVector AimPoint = FVector::ZeroVector;

	const bool bHasTarget =
		TryConsumeLockedTarget(TargetActor, TargetTags, AimPoint);

	/* =========================
	   방향 계산
	========================= */

	if (bHasTarget)
	{
		const FVector ToTarget = (AimPoint - SpawnLocation).GetSafeNormal();

		if (!ToTarget.IsNearlyZero())
		{
			ShootDirection = ToTarget;
		}
	}
	else
	{
		FVector CenterAimPoint = FVector::ZeroVector;

		if (BuildFallbackAimPoint(CenterAimPoint))
		{
			const FVector ToAim = (CenterAimPoint - SpawnLocation).GetSafeNormal();

			if (!ToAim.IsNearlyZero())
			{
				ShootDirection = ToAim;
			}
		}
	}
	
	/* =========================
	   4타 강화 데미지 결정
	========================= */

	float FinalProjectileDamage = ProjectileDamage;

	// 0=1타, 1=2타, 2=3타, 3=4타
	if (CurrentComboStep == 3)
	{
		FinalProjectileDamage = FourthComboProjectileDamage;
	}

	/* =========================
	   Actor Spawn
	========================= */

	FTransform SpawnTransform(
		ShootDirection.Rotation(),
		SpawnLocation
	);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarCharacter;
	SpawnParams.Instigator = AvatarCharacter;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABasicAttackActor* Projectile =
		World->SpawnActor<ABasicAttackActor>(
			BasicAttackActorClass,
			SpawnTransform,
			SpawnParams
		);

	if (!Projectile)
	{
		return;
	}

	// 같은 타수에서 성공적으로 1발 생성했으므로 이후 중복 생성 차단
	bProjectileFiredThisStep = true;

	/* =========================
	   Projectile 초기화
	========================= */

	Projectile->InitProjectile(
		AvatarCharacter,
		TargetActor,
		FinalProjectileDamage
	);

	Projectile->FireToDirection(ShootDirection);

	if (CastSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CastSound, AvatarCharacter->GetActorLocation());
	}
}

bool UGA_Spell_BasicAttack::BuildTraceStartEnd(
	const FGameplayAbilityActorInfo* ActorInfo,
	FVector& OutStart,
	FVector& OutEnd,
	AActor*& OutLockTarget
) const
{
	OutLockTarget = nullptr;

	const float Range = GetCastRange();
	if (Range <= 0.f)
	{
		return false;
	}

	AActor* TargetActor = nullptr;
	FGameplayTagContainer TargetTags;
	FVector AimPoint = FVector::ZeroVector;

	const bool bHasTarget =
		TryConsumeLockedTarget(TargetActor, TargetTags, AimPoint);

	FVector CenterAim;
	if (!BuildFallbackAimPoint(CenterAim, Range))
	{
		return false;
	}

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar)
	{
		return false;
	}

	OutStart = Avatar->GetActorLocation();

	if (bHasTarget && IsValid(TargetActor))
	{
		OutLockTarget = TargetActor;
		OutEnd = TargetActor->GetActorLocation();
	}
	else
	{
		OutEnd = AimPoint.IsNearlyZero() ? CenterAim : AimPoint;
	}

	return true;
}

void UGA_Spell_BasicAttack::FireHitScan(const FGameplayAbilityActorInfo* ActorInfo)
{
	UWorld* World = GetWorld();
	if (!World || !ActorInfo)
	{
		return;
	}

	AActor* Avatar = ActorInfo->AvatarActor.Get();
	if (!Avatar)
	{
		return;
	}

	FVector Start, End;
	AActor* LockTarget = nullptr;

	if (!BuildTraceStartEnd(ActorInfo, Start, End, LockTarget))
	{
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(HOG_BasicAttackTrace), false);
	if (bIgnoreSelf)
	{
		Params.AddIgnoredActor(Avatar);
	}

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, TraceChannel, Params);

	const float Damage = GetBaseDamage();

	if (bHit && Hit.GetActor())
	{
	}
}

bool UGA_Spell_BasicAttack::IsComboInputBufferedValid() const
{
	if (!bNextComboQueued)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const float CurrentTime = World->GetTimeSeconds();
	const float Elapsed = CurrentTime - LastComboQueuedTime;

	return Elapsed <= ComboInputBufferSeconds;
}