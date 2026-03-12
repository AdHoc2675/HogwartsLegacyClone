// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/Enemy/GA_GoblinMeleeAttack.h"

#include "Character/Enemy/EnemyCharacterBase.h"
#include "Data/Enemy/DA_GoblinConfig.h"

float UGA_GoblinMeleeAttack::GetMeleeAttackDamage() const
{
	const FGoblinAttackData* Data = GetAttackData();
	return Data ? Data->Damage : 1.f;
}

UAnimMontage* UGA_GoblinMeleeAttack::GetAttackMontage() const
{
	const FGoblinAttackData* Data = GetAttackData();
	return Data ? Data->Montage : nullptr;
}

const FGoblinAttackData* UGA_GoblinMeleeAttack::GetAttackData() const
{
	AEnemyCharacterBase* Enemy = Cast<AEnemyCharacterBase>(GetCharacter());
	if (!Enemy) return nullptr;
	
	UDA_GoblinConfig* Config = Cast<UDA_GoblinConfig>(Enemy->GetEnemyConfig());
	if (!Config) return nullptr;
	
	FGameplayTag Tag = AbilityTags.First();
	return Tag.IsValid() ? Config->FindAttackData(Tag) : nullptr;
}
