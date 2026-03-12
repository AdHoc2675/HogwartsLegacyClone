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


void AGoblinEnemyCharacter::GetMeleeAttackRange(FName AttackTag,
	float& OutMinRange, float& OutMaxRange) const
{
	OutMinRange = 0.f;
	OutMaxRange = 120.f;

	if (!GoblinConfig) return;

	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(AttackTag);
	const FGoblinAttackData* AttackData = GoblinConfig->FindAttackData(Tag);
	if (!AttackData) return;

	OutMinRange = AttackData->MinRange;
	OutMaxRange = AttackData->MaxRange;
}

TArray<FGameplayTag> AGoblinEnemyCharacter::GetMeleeAttackTags() const
{
	return GoblinConfig ? GoblinConfig->MeleeAttackTags : TArray<FGameplayTag>();
}
