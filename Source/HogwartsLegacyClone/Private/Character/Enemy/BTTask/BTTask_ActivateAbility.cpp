// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/BTTask_ActivateAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AIController.h"

UBTTask_ActivateAbility::UBTTask_ActivateAbility()
{
    NodeName = "Activate Ability";
    bNotifyTaskFinished = true;
    bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_ActivateAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC) return EBTNodeResult::Failed;
    
    APawn* Pawn = AIC->GetPawn();
    if (!Pawn) return EBTNodeResult::Failed;
    
    IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn);
    if (!ASI) return EBTNodeResult::Failed;
    
    AbilitySystem = ASI->GetAbilitySystemComponent();
    if (!AbilitySystem.IsValid()) return EBTNodeResult::Failed;
    
    FGameplayTagContainer TagContainer;
    TagContainer.AddTag(AbilityTag);
    
    // 어빌리티 시도
    bool bSuccess = AbilitySystem->TryActivateAbilitiesByTag(TagContainer);
    if (!bSuccess) return EBTNodeResult::Failed;
    
    BehaviourTree = &OwnerComp;
    AbilityEndedHandle = AbilitySystem->OnAbilityEnded.AddUObject(this, &UBTTask_ActivateAbility::OnAbilityEnded);
    
    return EBTNodeResult::InProgress;
}

void UBTTask_ActivateAbility::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
    if (AbilitySystem.IsValid())
    {
        AbilitySystem->OnAbilityEnded.Remove(AbilityEndedHandle);
        AbilitySystem.Reset();
    }
    
    BehaviourTree.Reset();
    AbilityEndedHandle.Reset();
    
    Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void UBTTask_ActivateAbility::OnAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
    if (!AbilityEndedData.AbilityThatEnded) return;
    if (!AbilityEndedData.AbilityThatEnded->AbilityTags.HasTag(AbilityTag)) return;
    
    if (BehaviourTree.IsValid())
    {
        FinishLatentTask(*BehaviourTree.Get(), EBTNodeResult::Succeeded);
    }
}

FString UBTTask_ActivateAbility::GetStaticDescription() const
{
    if (!AbilityTag.IsValid())
    {
        return TEXT("Activate Ability\n[No Tag Set]");
    }

    return FString::Printf(TEXT("Activate Ability\nTag: %s"), *AbilityTag.ToString());
}