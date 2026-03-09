// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Protego.generated.h"

/**
 * Protego 활성 상태용 GameplayEffect
 * - 일정 시간 동안 Protego 활성 태그 부여
 * - 실제 방어 판정은 ProtegoActor / 추후 전투 판정 로직에서 처리
 */

UCLASS()
class HOGWARTSLEGACYCLONE_API UGE_Protego : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Protego();
};
