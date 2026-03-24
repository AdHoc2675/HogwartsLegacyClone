#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "GameplayTagContainer.h"
#include "BTDecorator_GameplayTagCondition.generated.h"

class UAbilitySystemComponent;

UCLASS()
class HOGWARTSLEGACYCLONE_API UBTDecorator_GameplayTagCondition : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_GameplayTagCondition();

	UPROPERTY(EditAnywhere, Category = "Condition")
	FGameplayTag TagToCheck;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual FString GetStaticDescription() const override;
    
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	UAbilitySystemComponent* GetASC(const UBehaviorTreeComponent& OwnerComp) const;
    
	UFUNCTION()
	void OnTagChanged(const FGameplayTag Tag, int32 NewCount);
    
	UPROPERTY()
	TWeakObjectPtr<UBehaviorTreeComponent> BTComp;
    
	FDelegateHandle TagChangedHandle;
};