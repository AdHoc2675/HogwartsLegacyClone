// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/Enemy/DA_GoblinConfig.h"
#include "GAS/Abilities/Enemy/GA_MeleeAttack.h"
#include "GA_GoblinMeleeAttack.generated.h"

enum class EGoblinAttackType : uint8;
struct FGoblinAttackData;
/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UGA_GoblinMeleeAttack : public UGA_MeleeAttack
{
	GENERATED_BODY()

	
	
protected:

	virtual float GetMeleeAttackDamage() const override;
	virtual UAnimMontage* GetAttackMontage() const override;

private:
	const FGoblinAttackData* GetAttackData() const;
};
