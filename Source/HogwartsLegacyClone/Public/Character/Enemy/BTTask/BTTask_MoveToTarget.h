// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_MoveToTarget.generated.h"

/**
 * 
 */
class AAIController;
class AEnemyCharacterBase;

UCLASS()
class HOGWARTSLEGACYCLONE_API UBTTask_MoveToTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_MoveToTarget();

protected:
	UPROPERTY()
	TWeakObjectPtr<AAIController> AIController;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	                            EBTNodeResult::Type TaskResult) override;

private:
	float GetAcceptanceRadius(UBlackboardComponent* BB, APawn* Pawn) const;
};
