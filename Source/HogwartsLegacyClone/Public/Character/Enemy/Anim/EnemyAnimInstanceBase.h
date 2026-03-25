// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EnemyAnimInstanceBase.generated.h"

class UAbilitySystemComponent;
class AEnemyCharacterBase;
/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UEnemyAnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	
	void SetDead();
	
protected:
	UPROPERTY()
	AEnemyCharacterBase* EnemyCharacter;
	
	UPROPERTY()
	UAbilitySystemComponent* AbilitySystemComponent;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Speed;
	
	UPROPERTY(BlueprintReadOnly, Category = "Locomotion")
	float Direction;
	
	UPROPERTY(BlueprintReadOnly, Category = "State")
	bool bIsDead;
};
