// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "HOG_PlayerState.generated.h"

class UHOGAbilitySystemComponent;
class UHOGAttributeSet;
class UDA_AbilitySet;
class USpellComponent;

/**
 * PlayerState
 * - ASC 호스트
 * - AttributeSet 보관
 * - AbilitySet 지급
 * - SpellComponent 보관
 */

UCLASS()
class HOGWARTSLEGACYCLONE_API AHOG_PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AHOG_PlayerState();

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category="HOG|GAS")
	UHOGAbilitySystemComponent* GetHOGAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category="HOG|GAS")
	UHOGAttributeSet* GetAttributeSet() const;
	
	UFUNCTION(BlueprintPure, Category="HOG|Spell")
	USpellComponent* GetSpellComponent() const;
	
protected:
	virtual void BeginPlay() override;

protected:
	// ASC
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="GAS")
	TObjectPtr<UHOGAbilitySystemComponent> AbilitySystemComponent;

	// Attributes
	UPROPERTY()
	TObjectPtr<UHOGAttributeSet> AttributeSet;

	// Spell Runtime
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Spell")
	TObjectPtr<USpellComponent> SpellComponent;

	// AbilitySet
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GAS")
	TObjectPtr<UDA_AbilitySet> AbilitySet;
};
