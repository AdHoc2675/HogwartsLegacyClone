// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "HOGAttributeSet.generated.h"

// GAS Attribute Accessor 매크로 (UE 표준)
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 최소 AttributeSet
 * - Health / MaxHealth / AttackPower
 *
 * 데미지 계산:
 * CombatComponent + GameplayEffectExecutionCalculation
 *
 * 싱글 우선로 확장성은 일단 없애도 만듦
 */



UCLASS()
class HOGWARTSLEGACYCLONE_API UHOGAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
	
public:
	
	UHOGAttributeSet();
	
	// -----------------------
	// Attributes
	// -----------------------
	UPROPERTY(BlueprintReadOnly, Category="HOG|Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UHOGAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category="HOG|Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UHOGAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category="HOG|Attributes")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(UHOGAttributeSet, AttackPower)

public:

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

private:

	void ClampHealth();
	
};
