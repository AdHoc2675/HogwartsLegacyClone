// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/Enemy/GA_EnemyDashAttack.h"

#include "AIController.h"
#include "GameFramework/Character.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Engine/OverlapResult.h"


void UGA_EnemyDashAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo,
                                          const FGameplayAbilityActivationInfo ActivationInfo,
                                          const FGameplayEventData* TriggerEventData)
{
	// if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	// {
	// 	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	// 	return;
	// }

	ACharacter* Character = GetCharacter();
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAnimMontage* Montage = GetAttackMontage();
	if (!Montage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}


	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, Montage, 1.0f, FName("Start"));
	
	MontageTask->OnCompleted.AddDynamic(this, &UGA_EnemyDashAttack::OnMontageCompleted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_EnemyDashAttack::OnMontageCancelled);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_EnemyDashAttack::OnMontageCancelled);

	MontageTask->ReadyForActivation();
	
	FVector Direction = Character->GetActorForwardVector();
	
	if (UWorld* World = Character->GetWorld())
	{
		APawn* PlayerPawn = World->GetFirstPlayerController()->GetPawn();
		if (PlayerPawn)
		{
			Direction = (PlayerPawn->GetActorLocation() - Character->GetActorLocation()).GetSafeNormal2D();
			Character->SetActorRotation(Direction.Rotation());
		}
	}

	UAbilityTask_ApplyRootMotionConstantForce* DashTask =
		UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
			this, NAME_None, Direction, DashForce, DashDuration, false, nullptr,
			ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, 0.f, true);

	DashTask->OnFinish.AddDynamic(this, &UGA_EnemyDashAttack::OnDashFinished);
	DashTask->ReadyForActivation();

	Character->GetWorldTimerManager().SetTimer(
		TimerHandle, this, &UGA_EnemyDashAttack::CheckBodyHit, 0.02f, true);
}

void UGA_EnemyDashAttack::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo,
                                     const FGameplayAbilityActivationInfo ActivationInfo,
                                     bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ACharacter* Character = GetCharacter())
	{
		Character->GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	}
	bHasHit = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_EnemyDashAttack::CheckBodyHit()
{
	// 이미 부딧쳤으면 리턴
	if (bHasHit) return;

	ACharacter* Character = GetCharacter();
	if (!Character)
	{
		Character->GetWorldTimerManager().ClearTimer(TimerHandle);
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams CollisionParams;
	CollisionParams.AddIgnoredActor(Character);

	bool bHit = Character->GetWorld()->OverlapMultiByChannel(
		Overlaps, Character->GetActorLocation(), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(DashHitRadius),
		CollisionParams);

	if (!bHit) return;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* HitActor = Overlap.GetActor();
		if (!HitActor) continue;

		ACharacter* HitCharacter = Cast<ACharacter>(HitActor);
		if (!HitCharacter) continue;

		// 팀 확인
		if (HitCharacter->ActorHasTag("Enemy")) continue;

		bHasHit = true;
		Character->GetWorld()->GetTimerManager().ClearTimer(TimerHandle);

		FGameplayEventData Payload;
		Payload.Target = HitActor;
		OnWeaponHit(Payload);
		return;
	}
}

void UGA_EnemyDashAttack::OnDashFinished()
{
	ACharacter* Character = GetCharacter();
	if (!Character) return;
	 
	UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
	if (!AnimInstance) return;
	
	AnimInstance->Montage_JumpToSection(FName("Stop"), GetAttackMontage());
	
	Character->GetWorldTimerManager().ClearTimer(TimerHandle);
}
