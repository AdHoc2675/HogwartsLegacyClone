// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DA_EnemyConfig.generated.h"

/**
 * 
 */
class UGameplayAbility;
class UGameplayEffect;
class UBehaviorTree;

UCLASS()
class HOGWARTSLEGACYCLONE_API UDA_EnemyConfig : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category = "Info")
	FName EnemyName;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> DefaultAttributes;

	UPROPERTY(EditDefaultsOnly, Category = "GAS")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "AI")
	UBehaviorTree* BehaviorTree;

	// UPROPERTY(EditDefaultsOnly, Category = "Combat")
	// float AttackRange = 150.f;
	//
	// UPROPERTY(EditDefaultsOnly, Category = "Combat")
	// float DetectionRange = 1500.f;
	
};
