// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/GE/GE_Damage.h"

#include "GAS/Abilities/GE/ExecCalc_Damage.h"

UGE_Damage::UGE_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition ExecutionDef;
	ExecutionDef.CalculationClass = UExecCalc_Damage::StaticClass();

	Executions.Add(ExecutionDef);
}
