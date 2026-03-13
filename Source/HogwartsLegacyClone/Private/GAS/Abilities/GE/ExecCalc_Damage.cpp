#include "GAS/Abilities/GE/ExecCalc_Damage.h"

#include "GAS/Attributes/HOGAttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Core/HOG_GameplayTags.h"
#include "GameplayTagContainer.h"
#include "HOGDebugHelper.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

struct FDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Health);

	FDamageStatics()
	{
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
	RelevantAttributesToCapture.Add(DamageStatics().HealthDef);
}

void UExecCalc_Damage::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput
) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	const FGameplayEffectContextHandle& ContextHandle = Spec.GetContext();

	AActor* ContextInstigator = ContextHandle.GetInstigator();
	AActor* ContextCauser = ContextHandle.GetEffectCauser();
	UObject* ContextSourceObject = ContextHandle.GetSourceObject();

	// Debug::Print(FString::Printf(
	// 	TEXT("[ExecCalc_Damage] Context Check | Instigator=%s | Causer=%s | SourceObject=%s"),
	// 	*GetNameSafe(ContextInstigator),
	// 	*GetNameSafe(ContextCauser),
	// 	*GetNameSafe(ContextSourceObject)
	// ), FColor::Cyan);

	const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	const UHOGAttributeSet* TargetAttrDirect = TargetASC ? TargetASC->GetSet<UHOGAttributeSet>() : nullptr;

	// Debug::Print(FString::Printf(
	// 	TEXT("[ExecCalc_Damage] Target ASC Check | TargetASC=%s | TargetAttr=%s"),
	// 	TargetASC ? *GetNameSafe(TargetASC->GetOwner()) : TEXT("Null"),
	// 	TargetAttrDirect ? TEXT("Valid") : TEXT("Null")
	// ), FColor::Cyan);

	const FGameplayTag DamageDataTag = HOGGameplayTags::Data_Damage;
	const float BaseDamage = Spec.GetSetByCallerMagnitude(DamageDataTag, false, 0.0f);

	/* =========================
	   Source Actor 결정
	========================= */

	AActor* ResolvedSourceActor = ContextInstigator;

	if (!ResolvedSourceActor)
	{
		ResolvedSourceActor = ContextCauser;
	}

	if (!ResolvedSourceActor)
	{
		ResolvedSourceActor = Cast<AActor>(ContextSourceObject);
	}

	/* =========================
	   Source ASC 탐색
	========================= */

	UAbilitySystemComponent* SourceASC = nullptr;

	if (ResolvedSourceActor)
	{
		SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ResolvedSourceActor);

		if (!SourceASC)
		{
			if (const APawn* SourcePawn = Cast<APawn>(ResolvedSourceActor))
			{
				if (APlayerState* SourcePS = SourcePawn->GetPlayerState())
				{
					SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourcePS);
				}
			}
		}
	}

	const UHOGAttributeSet* SourceAttrDirect = SourceASC ? SourceASC->GetSet<UHOGAttributeSet>() : nullptr;

	// Debug::Print(FString::Printf(
	// 	TEXT("[ExecCalc_Damage] Source Resolve | SourceActor=%s | SourceASC=%s | SourceAttr=%s"),
	// 	*GetNameSafe(ResolvedSourceActor),
	// 	SourceASC ? *GetNameSafe(SourceASC->GetOwner()) : TEXT("Null"),
	// 	SourceAttrDirect ? TEXT("Valid") : TEXT("Null")
	// ), FColor::Yellow);

	float SourceAttackPower = 0.0f;

	if (SourceAttrDirect)
	{
		SourceAttackPower = SourceAttrDirect->GetAttackPower();

		// Debug::Print(FString::Printf(
		// 	TEXT("[ExecCalc_Damage] Direct Source AttackPower = %.2f"),
		// 	SourceAttackPower
		// ), FColor::Yellow);
	}
	else
	{
		// Debug::Print(TEXT("[ExecCalc_Damage] SourceAttrDirect is null"), FColor::Red);
	}

	const float FinalDamage = FMath::Max(BaseDamage + SourceAttackPower, 0.0f);

	// Debug::Print(FString::Printf(
	// 	TEXT("[ExecCalc_Damage] Execute | BaseDamage=%.2f | SourceAttackPower=%.2f | FinalDamage=%.2f"),
	// 	BaseDamage,
	// 	SourceAttackPower,
	// 	FinalDamage
	// ), FColor::Orange);

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

	// Debug::Print(TEXT("[ExecCalc_Damage] Health modifier applied"), FColor::Green);
}