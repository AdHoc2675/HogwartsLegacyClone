// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/NoExportTypes.h"
#include "AttackInfoProvider.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UAttackInfoProvider : public UObject
{
	GENERATED_BODY()
	
public:
	static bool GetRange(APawn* Pawn, FName AttackTag, float& OutMin, float& OutMax);
	static TArray<FGameplayTag> GetAllTags(APawn* Pawn);
	
};
