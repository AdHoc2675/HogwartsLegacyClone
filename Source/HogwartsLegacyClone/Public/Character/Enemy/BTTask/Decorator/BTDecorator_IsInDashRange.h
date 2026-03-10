// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_IsInDashRange.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UBTDecorator_IsInDashRange : public UBTDecorator
{
	GENERATED_BODY()
	
public:
	UBTDecorator_IsInDashRange();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;
};
