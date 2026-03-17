// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Stun.generated.h"

/**
 * 공용 기절 GameplayEffect
 * - Duration 기반
 * - 적용 중 대상에게 State.Stunned 태그를 부여
 *
 * 실제 지속시간은 기본값을 두고,
 * BP 자식(BP_GE_Stun)에서 튜닝하는 방식 권장
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UGE_Stun : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Stun();
};