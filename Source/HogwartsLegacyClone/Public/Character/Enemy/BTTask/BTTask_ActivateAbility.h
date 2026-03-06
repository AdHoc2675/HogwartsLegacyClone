// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ActivateAbility.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UBTTask_ActivateAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ActivateAbility();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
	
protected:
	// 실행할 어빌리티 태그
	UPROPERTY(EditAnywhere, Category = "Ability")
	FGameplayTag AbilityTag;
};
