// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/GE/ExecCalc_Damage.h"

#include "GAS/Attributes/HOGAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "GameplayTagContainer.h"

struct FDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);

	FDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(UHOGAttributeSet, AttackPower, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UHOGAttributeSet, Health, Target, false);
	}
};

static const FDamageStatics& DamageStatics()
{
	static FDamageStatics Statics;
	return Statics;
}

UExecCalc_Damage::UExecCalc_Damage()
{
	RelevantAttributesToCapture.Add(DamageStatics().AttackPowerDef);
	RelevantAttributesToCapture.Add(DamageStatics().HealthDef);
}

void UExecCalc_Damage::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput
) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluationParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	static const FGameplayTag DamageDataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"));

	const float BaseDamage = Spec.GetSetByCallerMagnitude(DamageDataTag, false, 0.0f);

	float SourceAttackPower = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		DamageStatics().AttackPowerDef,
		EvaluationParameters,
		SourceAttackPower
	);

	// 1차 버전:
	// 현재는 CombatComponent가 넘긴 BaseDamage만 그대로 사용.
	// AttackPower는 캡처만 해두고 아직 계산에는 넣지 않음.
	// 나중에 필요하면 여기서:
	// FinalDamage = BaseDamage + SourceAttackPower;
	const float FinalDamage = FMath::Max(BaseDamage, 0.0f);

	if (FinalDamage <= 0.0f)
	{
		return;
	}

	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(
			DamageStatics().HealthProperty,
			EGameplayModOp::Additive,
			-FinalDamage
		)
	);
}