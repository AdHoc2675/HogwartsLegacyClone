// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "Interface/IMeleeAttacker.h"
#include "GoblinEnemyCharacter.generated.h"

class UDA_GoblinConfig;
/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API AGoblinEnemyCharacter : public AEnemyCharacterBase, public IIMeleeAttacker
{
	GENERATED_BODY()
public:
	virtual void OnHealthChanged(float OldValue, float NewValue) override;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TObjectPtr<UDA_GoblinConfig> GoblinConfig;
	
	virtual UDA_EnemyConfigBase* GetEnemyConfig() const override;
	virtual void GetMeleeAttackRange(FName AttackTag, float& OutMinRange, float& OutMaxRange) const override;
	virtual TArray<FGameplayTag> GetMeleeAttackTags() const override;
};
