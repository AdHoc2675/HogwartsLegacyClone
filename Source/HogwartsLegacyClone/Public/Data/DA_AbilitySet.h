// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "DA_AbilitySet.generated.h"


class UGameplayAbility;
class UAbilitySystemComponent;

USTRUCT(BlueprintType)
struct FHOGAbilitySet
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayAbility>Ability;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag InputTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 AbilityLevel=1;
	
};

/**
 * 
 */
UCLASS(BlueprintType)
class HOGWARTSLEGACYCLONE_API UDA_AbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FHOGAbilitySet> Abilities;
	
	void GiveAbilities(UAbilitySystemComponent* ASC) const;
};
