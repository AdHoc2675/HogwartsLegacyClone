// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SetChaseDelay.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UBTTask_SetChaseDelay : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SetChaseDelay();

	UPROPERTY(EditAnywhere, Category = "DelayTime")
	float DelayTime = 3.0f;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	                            EBTNodeResult::Type TaskResult) override;

private:
	UPROPERTY()
	FTimerHandle TimerHandle;
};
