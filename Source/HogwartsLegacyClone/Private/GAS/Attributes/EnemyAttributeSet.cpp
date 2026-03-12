// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Attributes/EnemyAttributeSet.h"

#include "GameplayEffectExtension.h"

void UEnemyAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		float Damage = GetIncomingDamage();
		SetIncomingDamage(0.f);
		SetHealth(FMath::Max(0.f, GetHealth() - Damage));
	}
}
