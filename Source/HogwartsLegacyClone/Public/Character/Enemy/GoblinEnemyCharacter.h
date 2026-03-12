// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MeleeEnemyCharacterBase.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "Interface/IMeleeAttacker.h"
#include "GoblinEnemyCharacter.generated.h"

class UDA_MeleeEnemyConfig;
/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API AGoblinEnemyCharacter : public AMeleeEnemyCharacterBase
{
	GENERATED_BODY()
public:
	virtual void OnHealthChanged(float OldValue, float NewValue) override;
	
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TObjectPtr<UDA_MeleeEnemyConfig> GoblinConfig;
	
	virtual UDA_EnemyConfigBase* GetEnemyConfig() const override;
};
