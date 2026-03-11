// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/GA_Base.h"
#include "GAS/HOGAbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemComponent.h"
#include "HOGDebugHelper.h"


UGA_Base::UGA_Base()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	bLogOnActivate = false;
	bLogOnEnd = false;
}

UHOGAbilitySystemComponent* UGA_Base::GetHOGASC() const
{
	if (!CurrentActorInfo)
	{
		return nullptr;
	}
	
	return Cast<UHOGAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get());
}

APlayerController* UGA_Base::GetPlayerController() const
{
	return CurrentActorInfo?Cast<APlayerController>(CurrentActorInfo->PlayerController.Get()):nullptr;
}

APawn* UGA_Base::GetPawn() const
{
	return CurrentActorInfo?Cast<APawn>(CurrentActorInfo->AvatarActor.Get()):nullptr;
}

ACharacter* UGA_Base::GetCharacter() const
{
	return CurrentActorInfo?Cast<ACharacter>(CurrentActorInfo->AvatarActor.Get()):nullptr;
}

APlayerState* UGA_Base::GetPlayerState() const
{
	if (APlayerController* PC = GetPlayerController())
	{
		return PC->PlayerState;
	}
	
	if (APawn* Pawn = GetPawn())
	{
		return Pawn->GetPlayerState();
	}

	return nullptr;
}

void UGA_Base::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (bLogOnActivate)
	{
		LogAbility(TEXT("[GA Activate]"), false);
	}
		
}

void UGA_Base::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bLogOnEnd)
	{
		LogAbility(TEXT("[GA End]"), bWasCancelled);
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Base::LogAbility(const TCHAR* Prefix, bool bWasCancelled) const
{
	const UObject* SourceObj = GetCurrentSourceObject();
	const AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;
	const AActor* Owner  = CurrentActorInfo ? CurrentActorInfo->OwnerActor.Get()  : nullptr;

	const FString Msg = FString::Printf(
		TEXT("%s %s | Cancelled=%s | InputTag=%s | Avatar=%s | Owner=%s | Source=%s"),
		Prefix,
		*GetNameSafe(this),
		bWasCancelled ? TEXT("true") : TEXT("false"),
		*InputTag.ToString(),
		*GetNameSafe(Avatar),
		*GetNameSafe(Owner),
		*GetNameSafe(SourceObj)
	);

	// // 취소면 빨강으로
	// Debug::Print(Msg, bWasCancelled ? FColor::Red : FColor::MakeRandomColor());
}
