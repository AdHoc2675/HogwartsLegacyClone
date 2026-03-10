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
	
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetActorKey;
	
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetDistanceKey;
	
	protected:
	UPROPERTY()
	TWeakObjectPtr<AAIController> AIController;
	
	UPROPERTY()
	TWeakObjectPtr<APawn> Pawn;
	
	UPROPERTY()
	TWeakObjectPtr<UBlackboardComponent> Blackboard;
	
	TWeakObjectPtr<AEnemyCharacterBase> Enemy;
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;
};
