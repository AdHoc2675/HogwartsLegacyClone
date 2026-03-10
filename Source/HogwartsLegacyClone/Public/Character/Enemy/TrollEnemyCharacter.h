// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "Interface/IDashable.h"
#include "Interface/IMeleeAttacker.h"
#include "TrollEnemyCharacter.generated.h"

class UDA_TrollConfig;
/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API ATrollEnemyCharacter : public AEnemyCharacterBase, public IIMeleeAttacker, public IIDashable
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TObjectPtr<UDA_TrollConfig> TrollConfig;

	virtual void OnHealthChanged(float OldValue, float NewValue) override;
	
	virtual UDA_EnemyConfigBase* GetEnemyConfig() const override;
	
	// IMeleeAttacker
	virtual float GetMeleeAttackRange() const override;
	virtual TArray<FGameplayTag> GetMeleeAttackTags() const override;

	// IDashable
	virtual float GetDashMinRange() const override;
	virtual float GetDashMaxRange() const override;
	virtual FGameplayTag GetDashAbilityTag() const override;
};
