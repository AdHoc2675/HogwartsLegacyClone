// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Attributes/HOGAttributeSet.h"
#include "EnemyAttributeSet.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UEnemyAttributeSet : public UHOGAttributeSet
{
	GENERATED_BODY()

public:
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	
	UPROPERTY(BlueprintReadOnly, Category="HOG|Attributes")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UEnemyAttributeSet, IncomingDamage)
	
};
