// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Leviosa.generated.h"

/**
 * Leviosa 공중 부양 상태용 GameplayEffect
 * - 대상에게 "State.Debuff.Levitated" 태그 부여
 * - 마법이 종료될 때(GA_Spell_Leviosa::EndAbility) 수동으로 제거
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UGE_Leviosa : public UGameplayEffect
{
	GENERATED_BODY()
	
public:
	UGE_Leviosa();
};
