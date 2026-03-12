// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "Interface/IMeleeAttacker.h"
#include "MeleeEnemyCharacterBase.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API AMeleeEnemyCharacterBase : public AEnemyCharacterBase, public IIMeleeAttacker
{
	GENERATED_BODY()

public:
	virtual void GetMeleeAttackRange(FName AttackTag, float& OutMinRange, float& OutMaxRange) const override;
	virtual TArray<FGameplayTag> GetMeleeAttackTags() const override;
};
