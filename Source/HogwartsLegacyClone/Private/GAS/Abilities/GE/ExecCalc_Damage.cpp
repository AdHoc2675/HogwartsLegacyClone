#include "GAS/Abilities/GE/ExecCalc_Damage.h"

#include "GAS/Attributes/HOGAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Core/HOG_GameplayTags.h"
#include "GameplayTagContainer.h"
#include "HOGDebugHelper.h"

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

	const FGameplayTag DamageDataTag = HOGGameplayTags::Data_Damage;

	const float BaseDamage = Spec.GetSetByCallerMagnitude(DamageDataTag, false, 0.0f);

	float SourceAttackPower = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		DamageStatics().AttackPowerDef,
		EvaluationParameters,
		SourceAttackPower
	);

	const float FinalDamage = FMath::Max(BaseDamage + SourceAttackPower, 0.0f);

	Debug::Print(FString::Printf(
		TEXT("[ExecCalc_Damage] Execute | BaseDamage=%.2f | SourceAttackPower=%.2f | FinalDamage=%.2f"),
		BaseDamage,
		SourceAttackPower,
		FinalDamage
	), FColor::Orange);

	if (FinalDamage <= 0.0f)
	{
		Debug::Print(TEXT("[ExecCalc_Damage] FinalDamage <= 0, return"), FColor::Red);
		return;
	}

	OutExecutionOutput.AddOutputModifier(
		FGameplayModifierEvaluatedData(
			DamageStatics().HealthProperty,
			EGameplayModOp::Additive,
			-FinalDamage
		)
	);

	Debug::Print(TEXT("[ExecCalc_Damage] Health modifier applied"), FColor::Green);
}