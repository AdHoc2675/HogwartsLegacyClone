// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/TrollEnemyCharacter.h"

#include "Data/Enemy/DA_TrollConfig.h"

void ATrollEnemyCharacter::OnHealthChanged(float OldValue, float NewValue)
{

}

UDA_EnemyConfigBase* ATrollEnemyCharacter::GetEnemyConfig() const
{
	return TrollConfig;
}


