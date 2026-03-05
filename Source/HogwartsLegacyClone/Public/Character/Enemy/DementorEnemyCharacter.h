// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "DementorEnemyCharacter.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API ADementorEnemyCharacter : public AEnemyCharacterBase
{
	GENERATED_BODY()
	
	virtual void OnHealthChanged(float OldValue, float NewValue) override;
	
};
