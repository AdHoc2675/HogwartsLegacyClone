// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Enemy/BTTask/Decorator/BTDecorator_GameplayTagCondition.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AIController.h"

UBTDecorator_GameplayTagCondition::UBTDecorator_GameplayTagCondition()
{
    NodeName = "GamePlayTag Condition";
    bNotifyBecomeRelevant = true;
    bNotifyCeaseRelevant = true;
    bCreateNodeInstance = true;
    FlowAbortMode = EBTFlowAbortMode::Both;
}

bool UBTDecorator_GameplayTagCondition::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    UAbilitySystemComponent* ASC = GetASC(OwnerComp);
    return ASC ? ASC->HasMatchingGameplayTag(TagToCheck) : false;
}

FString UBTDecorator_GameplayTagCondition::GetStaticDescription() const
{
    FString Prefix = IsInversed() ? TEXT("NOT ") : TEXT("");
    return FString::Printf(TEXT("%sHas Tag: %s"), *Prefix, *TagToCheck.ToString());
}

void UBTDecorator_GameplayTagCondition::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    Super::OnBecomeRelevant(OwnerComp, NodeMemory);
    
    BTComp = &OwnerComp;
    
    UAbilitySystemComponent* ASC = GetASC(OwnerComp);
    if (!ASC) return;

    TagChangedHandle = ASC->RegisterGameplayTagEvent(TagToCheck, EGameplayTagEventType::NewOrRemoved)
        .AddUObject(this, &UBTDecorator_GameplayTagCondition::OnTagChanged);
}

void UBTDecorator_GameplayTagCondition::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (TagChangedHandle.IsValid())
    {
        if (UAbilitySystemComponent* ASC = GetASC(OwnerComp))
        {
            ASC->UnregisterGameplayTagEvent(TagChangedHandle, TagToCheck);
        }
        TagChangedHandle.Reset();
    }
    
    BTComp.Reset();
    
    Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

void UBTDecorator_GameplayTagCondition::OnTagChanged(const FGameplayTag Tag, int32 NewCount)
{
    if (BTComp.IsValid())
    {
        BTComp->RequestExecution(this);
    }
}

UAbilitySystemComponent* UBTDecorator_GameplayTagCondition::GetASC(const UBehaviorTreeComponent& OwnerComp) const
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    if (!AIController) return nullptr;

    IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(AIController->GetPawn());
    if (!ASCInterface) return nullptr;

    return ASCInterface->GetAbilitySystemComponent();
}