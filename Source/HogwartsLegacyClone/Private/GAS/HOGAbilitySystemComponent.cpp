// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/HOGAbilitySystemComponent.h"
#include  "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbility.h"

UHOGAbilitySystemComponent::UHOGAbilitySystemComponent()
{
}

void UHOGAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> Handles;
	GetAbilitySpecHandlesByInputTag(InputTag, Handles);

	for (const FGameplayAbilitySpecHandle& Handle : Handles)
	{
		if (!Handle.IsValid())
		{
			continue;
		}

		InputPressedSpecHandles.AddUnique(Handle);
		InputHeldSpecHandles.AddUnique(Handle);
	}
}

void UHOGAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
	if (!InputTag.IsValid())
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> Handles;
	GetAbilitySpecHandlesByInputTag(InputTag, Handles);

	for (const FGameplayAbilitySpecHandle& Handle : Handles)
	{
		if (!Handle.IsValid())
		{
			continue;
		}

		InputReleasedSpecHandles.AddUnique(Handle);
		InputHeldSpecHandles.Remove(Handle);
	}
}

void UHOGAbilitySystemComponent::ProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	// Pressed:
	// - 비활성 상태면 Activate 시도
	// - 이미 활성 상태면 Ability 쪽으로 "입력 눌림" 이벤트 전달
	for (const FGameplayAbilitySpecHandle& Handle : InputPressedSpecHandles)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
		if (!Spec)
		{
			continue;
		}

		Spec->InputPressed = true;

		if (Spec->IsActive())
		{
			AbilitySpecInputPressed(*Spec);
		}
		else
		{
			TryActivateAbility(Handle);
		}
	}

	// Held: 누르고 있는 동안 (차지/채널링 같은 것 대비)
	for (const FGameplayAbilitySpecHandle& Handle : InputHeldSpecHandles)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
		if (!Spec) continue;

		// 이미 활성화되어 있으면 유지 입력 처리 (필요 시 확장)
		// 여기서는 최소로 유지
	}

	// Released: 입력 뗌
	for (const FGameplayAbilitySpecHandle& Handle : InputReleasedSpecHandles)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
		if (!Spec) continue;

		Spec->InputPressed = false;
		AbilitySpecInputReleased(*Spec);
	}

	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

void UHOGAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}

void UHOGAbilitySystemComponent::GetAbilitySpecHandlesByInputTag(const FGameplayTag& InputTag,
	TArray<FGameplayAbilitySpecHandle>& OutHandles)
{
	OutHandles.Reset();

	if (!InputTag.IsValid())
	{
		return;
	}

	// ActivatableAbilities는 UAbilitySystemComponent가 가진 모든 AbilitySpec 컨테이너
	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (!Spec.Handle.IsValid())
		{
			continue;
		}

		// InputTag = SpellID 정책이면, GiveAbility할 때 Spec.DynamicAbilityTags에 넣은 태그로 매칭 가능
		// (또는 Ability CDO의 AbilityTags로도 매칭 가능)
		const bool bMatchDynamic = Spec.DynamicAbilityTags.HasTagExact(InputTag);
		const bool bMatchAbilityTags = (Spec.Ability != nullptr) && Spec.Ability->AbilityTags.HasTagExact(InputTag);

		if (bMatchDynamic || bMatchAbilityTags)
		{
			OutHandles.Add(Spec.Handle);
		}
	}
}
