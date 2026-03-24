// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemComponent.h"

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
	
	UPROPERTY(EditAnywhere, Category = "Ability")
	FGameplayTag AbilityTag;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual FString GetStaticDescription() const override;

private:
	void OnAbilityEnded(const FAbilityEndedData& AbilityEndedData);
    
	TWeakObjectPtr<UBehaviorTreeComponent> BehaviourTree;
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
	
	FGameplayTag ActiveAbilityTag;
	FDelegateHandle AbilityEndedHandle;
};
