// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_IsInMeleeRange.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UBTDecorator_IsInMeleeRange : public UBTDecorator
{
	GENERATED_BODY()
	
public:
	UBTDecorator_IsInMeleeRange();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetDistanceKey;
	
	// UPROPERTY(EditAnywhere, Category = "Blackboard")
	// FBlackboardKeySelector TargetActorKey;
	
	UPROPERTY(EditAnywhere, Category = "Attack")
	FGameplayTag DirectAttackTag;
	
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector AttackTagKey;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;
	
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	mutable bool bLastConditionResult = false;
	mutable bool bFirstCheck = true;
};
