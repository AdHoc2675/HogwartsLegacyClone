// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/HOGAbilitySystemComponent.h"
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
	for (const FGameplayAbilitySpecHandle& Handle : InputPressedSpecHandles)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
		if (!Spec)
		{
			continue;
		}

		Spec->InputPressed = true;
		TryActivateAbility(Handle);
	}

	for (const FGameplayAbilitySpecHandle& Handle : InputReleasedSpecHandles)
	{
		FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
		if (!Spec)
		{
			continue;
		}

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
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
}
