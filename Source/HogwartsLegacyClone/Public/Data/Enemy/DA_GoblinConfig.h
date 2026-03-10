// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Data/Enemy/DA_EnemyConfigBase.h"
#include "DA_GoblinConfig.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UDA_GoblinConfig : public UDA_EnemyConfigBase
{
	GENERATED_BODY()
	
	
public:
	// 근접
	UPROPERTY(EditAnywhere, Category = "Melee")
	float MeleeAttackRange = 120.f;

	UPROPERTY(EditAnywhere, Category = "Melee")
	TArray<FGameplayTag> MeleeAttackTags;
};
