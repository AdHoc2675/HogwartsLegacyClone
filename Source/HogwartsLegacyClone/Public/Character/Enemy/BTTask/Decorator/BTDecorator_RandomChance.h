// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_RandomChance.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UBTDecorator_RandomChance : public UBTDecorator
{
	GENERATED_BODY()
public:
	UBTDecorator_RandomChance();
	
	UPROPERTY(EditAnywhere, Category = "Random", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Chance = 0.5f;
	
protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	
	virtual FString GetStaticDescription() const override;
};
