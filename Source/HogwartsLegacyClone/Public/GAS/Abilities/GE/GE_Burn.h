// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Burn.generated.h"

/**
 * 화상(Burn) 상태이상용 GameplayEffect 베이스
 * - 기본적으로 Duration + Period 기반 DoT 용도로 사용
 * - 실제 수치(지속시간, 주기, 데미지)는 BP 파생 클래스에서 세팅
 */

UCLASS()
class HOGWARTSLEGACYCLONE_API UGE_Burn : public UGameplayEffect
{
	GENERATED_BODY()
	
public:
	UGE_Burn();
	
};
