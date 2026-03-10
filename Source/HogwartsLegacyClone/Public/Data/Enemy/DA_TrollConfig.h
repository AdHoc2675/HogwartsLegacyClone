// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Data/Enemy/DA_EnemyConfigBase.h"
#include "DA_TrollConfig.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UDA_TrollConfig : public UDA_EnemyConfigBase
{
	GENERATED_BODY()
	
public:
	// 근접
	UPROPERTY(EditAnywhere, Category = "Melee")
	float MeleeAttackRange = 300.f;

	UPROPERTY(EditAnywhere, Category = "Melee")
	TArray<FGameplayTag> MeleeAttackTags;

	// 돌진
	UPROPERTY(EditAnywhere, Category = "Dash")
	float DashMinRange = 500.f;

	UPROPERTY(EditAnywhere, Category = "Dash")
	float DashMaxRange = 1200.f;

	UPROPERTY(EditAnywhere, Category = "Dash")
	FGameplayTag DashAbilityTag;
	
};
