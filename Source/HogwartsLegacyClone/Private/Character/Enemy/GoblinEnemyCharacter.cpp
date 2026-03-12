// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/GoblinEnemyCharacter.h"

#include "Data/Enemy/DA_MeleeEnemyConfig.h"

void AGoblinEnemyCharacter::OnHealthChanged(float OldValue, float NewValue)
{

}

UDA_EnemyConfigBase* AGoblinEnemyCharacter::GetEnemyConfig() const
{
	return GoblinConfig;
}