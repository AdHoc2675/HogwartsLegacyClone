// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/Decorator/BTDecorator_RandomChance.h"

UBTDecorator_RandomChance::UBTDecorator_RandomChance()
{
	NodeName = "Random Chance";
}

bool UBTDecorator_RandomChance::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	return FMath::FRand() <= Chance;
}

FString UBTDecorator_RandomChance::GetStaticDescription() const
{
	return FString::Printf(TEXT("Random Chance: %.0f%%"), Chance * 100.f);
}
