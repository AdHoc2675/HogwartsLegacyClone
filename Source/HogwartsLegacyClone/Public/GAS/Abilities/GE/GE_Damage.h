// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Damage.generated.h"

/**
 * 기본 데미지 GameplayEffect
 * - Instant
 * - ExecCalc_Damage 실행용
 */

UCLASS()
class HOGWARTSLEGACYCLONE_API UGE_Damage : public UGameplayEffect
{
	GENERATED_BODY()
	
public:
	UGE_Damage();
};
