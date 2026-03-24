// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SetRandomAttackTag.generated.h"

enum class EGoblinAttackType : uint8;
/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UBTTask_SetRandomAttackTag : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SetRandomAttackTag();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	virtual FString GetStaticDescription() const override;

private:
	
	// UPROPERTY(EditAnywhere, Category = "Blackboard")
	// FBlackboardKeySelector AbilityTagKey;
	
	UPROPERTY(EditAnywhere, Category = "Filter")
	FGameplayTagContainer ExcludeTags;
};
