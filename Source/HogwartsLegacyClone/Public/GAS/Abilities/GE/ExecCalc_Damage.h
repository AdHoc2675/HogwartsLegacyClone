#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "ExecCalc_Damage.generated.h"

/**
 * 데미지 실행 계산기
 * - SetByCaller(Data.Damage) 값을 읽고
 * - Source AttackPower를 더해서
 * - Target의 Health를 감소시키는 OutputModifier를 생성
 */

UCLASS()
class HOGWARTSLEGACYCLONE_API UExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UExecCalc_Damage();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput
	) const override;
};