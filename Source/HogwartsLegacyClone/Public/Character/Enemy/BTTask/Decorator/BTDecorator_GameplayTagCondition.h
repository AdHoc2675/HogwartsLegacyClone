// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_GameplayTagCondition.generated.h"

/**
 * 
 */
class UAbilitySystemComponent;
UCLASS()
class HOGWARTSLEGACYCLONE_API UBTDecorator_GameplayTagCondition : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_GameplayTagCondition();
	
protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	UPROPERTY(EditAnywhere, Category = "Tag")
	FGameplayTag TagToCheck;
	
	TWeakObjectPtr<UAbilitySystemComponent> AbilityComponent;
	FDelegateHandle TagChangedHandle;
};
