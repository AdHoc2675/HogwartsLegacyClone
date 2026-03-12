// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "IMeleeAttacker.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UIMeleeAttacker : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class HOGWARTSLEGACYCLONE_API IIMeleeAttacker
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	//virtual void GetMeleeAttackRange(int32 AttackTypeInt, float& OutMinRange, float& OutMaxRange) const = 0;
	virtual void GetMeleeAttackRange(FName AttackTag, float& OutMinRange, float& OutMaxRange) const = 0;
	
	virtual TArray<FGameplayTag> GetMeleeAttackTags() const = 0;
};
