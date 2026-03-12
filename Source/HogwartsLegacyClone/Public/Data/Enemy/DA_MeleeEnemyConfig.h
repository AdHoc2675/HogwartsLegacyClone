// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FEnemyAttackData.h"
#include "GameplayTagContainer.h"
#include "Data/Enemy/DA_EnemyConfigBase.h"
#include "DA_MeleeEnemyConfig.generated.h"

/**
 * 
 */

USTRUCT()
struct FGoblinAttackData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	UAnimMontage* Montage = nullptr;
	
	UPROPERTY(EditAnywhere)
	float Damage;
	
	UPROPERTY(EditAnywhere, Category = "Melee")
	float MinRange;
	
	UPROPERTY(EditAnywhere, Category = "Melee")
	float MaxRange;
	
	UPROPERTY(EditAnywhere)
	FGameplayTag AbilityTag;
	
};
UCLASS()
class HOGWARTSLEGACYCLONE_API UDA_MeleeEnemyConfig : public UDA_EnemyConfigBase
{
	GENERATED_BODY()
	
	
public:
	// 근접
	// UPROPERTY(EditAnywhere, Category = "Melee")
	// float MeleeAttackRange = 120.f;
	//
	// UPROPERTY(EditAnywhere, Category = "Melee")
	// float MeleeAttackDamage = 20.f;
	//
	// UPROPERTY(EditAnywhere, Category = "Melee")
	// TArray<FGameplayTag> MeleeAttackTags;
	
	UPROPERTY(EditAnywhere, Category = "Melee")
	TArray<FEnemyAttackData> MeleeAttacks;

	const FEnemyAttackData* FindAttackData(FGameplayTag Tag) const
	{
		return MeleeAttacks.FindByPredicate([Tag](const FEnemyAttackData& Data)
		{
			return Data.AbilityTag == Tag;
		});
	}
	
	TArray<FGameplayTag> GetMeleeAttackTags() const
	{
		TArray<FGameplayTag> Tags;
		for (const auto& Data : MeleeAttacks)
		{
			Tags.Add(Data.AbilityTag);
		}
		return Tags;
	}
};
