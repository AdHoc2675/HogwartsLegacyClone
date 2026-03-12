// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MeleeEnemyCharacterBase.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "Data/Enemy/DA_MeleeEnemyConfig.h"   
#include "TrollEnemyCharacter.generated.h"

class UDA_TrollConfig;
/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API ATrollEnemyCharacter : public AMeleeEnemyCharacterBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category = "Config")
	TObjectPtr<UDA_MeleeEnemyConfig> TrollConfig;

	virtual void OnHealthChanged(float OldValue, float NewValue) override;
	
	virtual UDA_EnemyConfigBase* GetEnemyConfig() const override;
	
};
