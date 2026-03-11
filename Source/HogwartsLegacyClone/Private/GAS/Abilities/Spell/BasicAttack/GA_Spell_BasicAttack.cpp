// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/Spell/BasicAttack/GA_Spell_BasicAttack.h"

#include "HOGDebugHelper.h"
#include "Data/DA_SpellDefinition.h"
#include "Character/Player/PlayerCharacterBase.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"

#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
#include "AbilitySystemComponent.h"

UGA_Spell_BasicAttack::UGA_Spell_BasicAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ComboIndex = 0;
	bComboInProgress = false;
	bNextComboQueued = false;
	CurrentComboStep = 0;
	bBranchConsumed = false;
	CurrentPlayingMontage = nullptr;
	bAdvancingComboFromBranch = false;
	LastComboQueuedTime=-1.f;
}

void UGA_Spell_BasicAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 첫 입력: 콤보 시작
	bComboInProgress = true;
	bNextComboQueued = false;
	CurrentComboStep = 0;
	bBranchConsumed = false;
	bAdvancingComboFromBranch = false;
	CurrentPlayingMontage = nullptr;

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
		Debug::Print(TEXT("[BasicAttack] InputPressed Ignored: Combo not in progress"), FColor::Orange);
		return;
	}

	if (!PlayerCharacter)
	{
		Debug::Print(TEXT("[BasicAttack] InputPressed Ignored: PlayerCharacter is null"), FColor::Red);
		return;
	}

	Debug::Print(FString::Printf(
		TEXT("[BasicAttack] InputPressed While Combo | CanQueue=%s | CurrentStep=%d"),
		PlayerCharacter->CanQueueNextCombo() ? TEXT("true") : TEXT("false"),
		CurrentComboStep
	), FColor::Yellow);

	if (PlayerCharacter->CanQueueNextCombo())
	{
		bNextComboQueued = true;

		if (UWorld* World = GetWorld())
		{
			LastComboQueuedTime = World->GetTimeSeconds();
		}

		Debug::Print(FString::Printf(
			TEXT("[BasicAttack] Next Combo Queued | CurrentStep=%d | Buffer=%.2f"),
			CurrentComboStep,
			ComboInputBufferSeconds
		), FColor::Yellow);
	}
	else
	{
		Debug::Print(TEXT("[BasicAttack] Combo Input Ignored: ComboWindow Closed"), FColor::Orange);
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

	// 현재 단계: 각 타 시작 즉시 타격 처리
	// 나중에 AttackHit Notify로 분리 가능
	FireHitScan(ActorInfo);
}

bool UGA_Spell_BasicAttack::TryPlayCurrentComboMontage(const FGameplayAbilityActorInfo* ActorInfo)
{
	ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (!Character)
	{
		return false;
	}

	if (!Character->GetMesh())
	{
		return false;
	}

	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return false;
	}

	if (!ComboMontages.IsValidIndex(CurrentComboStep))
	{
		return false;
	}

	UAnimMontage* MontageToPlay = ComboMontages[CurrentComboStep].Get();
	if (!MontageToPlay)
	{
		return false;
	}

	bBranchConsumed = false;
	bAdvancingComboFromBranch = false;

	AnimInstance->Montage_Stop(0.05f);

	const float Duration = AnimInstance->Montage_Play(MontageToPlay, 1.f);

	Debug::Print(FString::Printf(
		TEXT("[BasicAttack] Montage_Play Result | Step=%d | Duration=%.3f"),
		CurrentComboStep,
		Duration
	), Duration > 0.f ? FColor::Green : FColor::Red);

	if (Duration <= 0.f)
	{
		return false;
	}

	CurrentPlayingMontage = MontageToPlay;

	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UGA_Spell_BasicAttack::OnMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, MontageToPlay);

	return true;
}

void UGA_Spell_BasicAttack::TryAdvanceComboFromBranchPoint()
{
	if (!bComboInProgress)
	{
		Debug::Print(TEXT("[BasicAttack] TryAdvanceComboFromBranchPoint Ignored: Combo not in progress"), FColor::Orange);
		return;
	}

	if (bBranchConsumed)
	{
		Debug::Print(TEXT("[BasicAttack] TryAdvanceComboFromBranchPoint Ignored: Branch already consumed"), FColor::Orange);
		return;
	}

	if (!IsComboInputBufferedValid())
	{
		Debug::Print(TEXT("[BasicAttack] TryAdvanceComboFromBranchPoint Ignored: No valid buffered combo"), FColor::Orange);
		return;
	}

	const int32 NextStep = CurrentComboStep + 1;
	const bool bHasNextStep = ComboMontages.IsValidIndex(NextStep);

	if (!bHasNextStep)
	{
		Debug::Print(FString::Printf(
			TEXT("[BasicAttack] TryAdvanceComboFromBranchPoint Ignored: No next step | CurrentStep=%d"),
			CurrentComboStep
		), FColor::Orange);
		return;
	}

	bBranchConsumed = true;
	bNextComboQueued = false;
	LastComboQueuedTime=-1.f;
	bAdvancingComboFromBranch = true;
	CurrentComboStep = NextStep;

	Debug::Print(FString::Printf(
		TEXT("[BasicAttack] Branch Advance Combo | NextStep=%d"),
		CurrentComboStep
	), FColor::Yellow);

	if (!TryPlayCurrentComboMontage(CurrentActorInfo))
	{
		Debug::Print(TEXT("[BasicAttack] Branch Advance Failed: TryPlayCurrentComboMontage failed"), FColor::Red);
		ResetComboState();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	FireHitScan(CurrentActorInfo);
}

void UGA_Spell_BasicAttack::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	Debug::Print(FString::Printf(
		TEXT("[BasicAttack] OnMontageEnded | Montage=%s | CurrentPlaying=%s | Step=%d | Queued=%s | Interrupted=%s | BranchAdvancing=%s"),
		*GetNameSafe(Montage),
		*GetNameSafe(CurrentPlayingMontage),
		CurrentComboStep,
		bNextComboQueued ? TEXT("true") : TEXT("false"),
		bInterrupted ? TEXT("true") : TEXT("false"),
		bAdvancingComboFromBranch ? TEXT("true") : TEXT("false")
	), bInterrupted ? FColor::Orange : FColor::Green);

	// 현재 추적 중인 몽타주와 다르면 무시
	if (Montage != CurrentPlayingMontage)
	{
		Debug::Print(TEXT("[BasicAttack] OnMontageEnded Ignored: Not current playing montage"), FColor::Orange);
		return;
	}

	// 브랜치 전환으로 이전 타가 끊긴 경우는 정상
	if (bInterrupted && bAdvancingComboFromBranch)
	{
		Debug::Print(TEXT("[BasicAttack] OnMontageEnded Ignored: Interrupted by combo branch advance"), FColor::Orange);
		return;
	}

	// 진짜 인터럽트면 종료
	if (bInterrupted)
	{
		ResetComboState();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 브랜치 노티파이를 놓쳤을 때 fallback
	const int32 NextStep = CurrentComboStep + 1;
	const bool bHasNextStep = ComboMontages.IsValidIndex(NextStep);

	if (IsComboInputBufferedValid() && bHasNextStep)
	{
		CurrentComboStep = NextStep;
		bNextComboQueued = false;
		LastComboQueuedTime=-1.f;

		Debug::Print(FString::Printf(
			TEXT("[BasicAttack] Advance Combo From MontageEnd | NextStep=%d"),
			CurrentComboStep
		), FColor::Yellow);

		if (!TryPlayCurrentComboMontage(CurrentActorInfo))
		{
			ResetComboState();
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			return;
		}

		FireHitScan(CurrentActorInfo);
		return;
	}

	Debug::Print(TEXT("[BasicAttack] Combo End"), FColor::Silver);

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
	LastComboQueuedTime=-1.f;

	Debug::Print(TEXT("[BasicAttack] ResetComboState"), FColor::Silver);
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

	FGameplayTagContainer TargetTags;
	FVector AimPoint;
	AActor* Target = nullptr;

	AcquireTargetFromLockOn(Target, TargetTags, AimPoint);

	FVector CenterAim;
	if (!GetCenterAimPoint(CenterAim, Range))
	{
		return false;
	}

	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar)
	{
		return false;
	}

	OutStart = Avatar->GetActorLocation();

	if (IsValid(Target))
	{
		OutLockTarget = Target;
		OutEnd = Target->GetActorLocation();
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

	if (bDrawDebugLine)
	{
		const FVector DebugEnd = bHit ? Hit.ImpactPoint : End;
		DrawDebugLine(World, Start, DebugEnd, bHit ? FColor::Red : FColor::Green, false, 1.0f, 0, 2.0f);
	}

	const float Damage = GetBaseDamage();

	if (bHit && Hit.GetActor())
	{
		Debug::Print(FString::Printf(
			TEXT("[BasicAttack] Hit=%s Damage=%.1f"),
			*GetNameSafe(Hit.GetActor()),
			Damage
		));
	}
	else
	{
		Debug::Print(TEXT("[BasicAttack] No Hit"));
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
	const float Elapsed=CurrentTime-LastComboQueuedTime;
	
	return Elapsed<=ComboInputBufferSeconds;
}
