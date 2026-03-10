// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/GoblinEnemyCharacter.h"

#include "Data/Enemy/DA_GoblinConfig.h"

void AGoblinEnemyCharacter::OnHealthChanged(float OldValue, float NewValue)
{

}

UDA_EnemyConfigBase* AGoblinEnemyCharacter::GetEnemyConfig() const
{
	return GoblinConfig;
}

float AGoblinEnemyCharacter::GetMeleeAttackRange() const
{
	return GoblinConfig ? GoblinConfig->MeleeAttackRange : 120.f;
}

TArray<FGameplayTag> AGoblinEnemyCharacter::GetMeleeAttackTags() const
{
	return GoblinConfig ? GoblinConfig->MeleeAttackTags : TArray<FGameplayTag>();
}
