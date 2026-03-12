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

void ATrollEnemyCharacter::GetMeleeAttackRange(FName AttackTag, float& OutMinRange, float& OutMaxRange) const
{
}

TArray<FGameplayTag> ATrollEnemyCharacter::GetMeleeAttackTags() const
{
	return TrollConfig ? TrollConfig->MeleeAttackTags : TArray<FGameplayTag>();
}

float ATrollEnemyCharacter::GetDashMinRange() const
{
	return TrollConfig ? TrollConfig->DashMinRange : 500.f;
}

float ATrollEnemyCharacter::GetDashMaxRange() const
{
	return TrollConfig ? TrollConfig->DashMaxRange : 1200.f;
}

FGameplayTag ATrollEnemyCharacter::GetDashAbilityTag() const
{
	return TrollConfig ? TrollConfig->DashAbilityTag : FGameplayTag();
}
