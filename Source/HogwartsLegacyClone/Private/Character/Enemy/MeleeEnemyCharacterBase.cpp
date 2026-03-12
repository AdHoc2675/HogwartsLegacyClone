// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/MeleeEnemyCharacterBase.h"

#include "Data/Enemy/DA_MeleeEnemyConfig.h"
#include "Data/Enemy/FEnemyAttackData.h"

void AMeleeEnemyCharacterBase::GetMeleeAttackRange(FName AttackTag, float& OutMinRange, float& OutMaxRange) const
{
	OutMinRange = 0.f;
	OutMaxRange = GetMinAttackRange();

	UDA_MeleeEnemyConfig* Config = Cast<UDA_MeleeEnemyConfig>(GetEnemyConfig());
	if (!Config) return;

	FGameplayTag Tag = FGameplayTag::RequestGameplayTag(AttackTag, false);
	if (!Tag.IsValid()) return;

	const FEnemyAttackData* Data = Config->FindAttackData(Tag);
	if (!Data) return;

	OutMinRange = Data->MinRange;
	OutMaxRange = Data->MaxRange;
}

TArray<FGameplayTag> AMeleeEnemyCharacterBase::GetMeleeAttackTags() const
{
	UDA_MeleeEnemyConfig* Config = Cast<UDA_MeleeEnemyConfig>(GetEnemyConfig());
	return Config ? Config->GetMeleeAttackTags() : TArray<FGameplayTag>();
}
