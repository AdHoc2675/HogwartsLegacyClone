// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "HOGAbilitySystemComponent.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UHOGAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
	
public:

	UHOGAbilitySystemComponent();

	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	void ProcessAbilityInput(float DeltaTime, bool bGamePaused);

	void ClearAbilityInput();

protected:

	void GetAbilitySpecHandlesByInputTag(const FGameplayTag& InputTag, TArray<FGameplayAbilitySpecHandle>& OutHandles);

private:

	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;

	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;

	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
	
};
