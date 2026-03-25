// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/Helper/AttackInfoProvider.h"

#include "Character/Enemy/Interface/IMeleeAttacker.h"

bool UAttackInfoProvider::GetRange(APawn* Pawn, FName AttackTag, float& OutMin, float& OutMax)
{
	if (!Pawn) return false;

	if (IIMeleeAttacker* Attacker = Cast<IIMeleeAttacker>(Pawn))
	{
		Attacker->GetMeleeAttackRange(AttackTag, OutMin, OutMax);
		return true;
	}
	return false;
}

TArray<FGameplayTag> UAttackInfoProvider::GetAllTags(APawn* Pawn)
{
	TArray<FGameplayTag> Tags;
	if (!Pawn) return Tags;

	if (IIMeleeAttacker* Attacker = Cast<IIMeleeAttacker>(Pawn))
	{
		return Attacker->GetMeleeAttackTags();
	}
	return Tags;
}
