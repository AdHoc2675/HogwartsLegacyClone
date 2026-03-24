// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/BTTask_ActivateAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/Enemy/Helper/AIBlackboardHelper.h"

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
	
	
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;
	
	// 직접 어빌리티를 부여하는 경우
	ActiveAbilityTag = AbilityTag;
	
	// 랜덤한 어빌리티 부여
	FName TagName;
	if (!ActiveAbilityTag.IsValid())
	{
		TagName = UAIBlackboardHelper::GetAbilityTagName(BB);
		
		if (!TagName.IsNone())
		{
			ActiveAbilityTag = FGameplayTag::RequestGameplayTag(TagName);
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("%s"), *TagName.ToString())
	
	if (!ActiveAbilityTag.IsValid()) return EBTNodeResult::Failed;
	
	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(ActiveAbilityTag);
	
	BehaviourTree = &OwnerComp;
	// 어빌리티가 종료될 때 노드 종료
	AbilityEndedHandle = AbilitySystem->OnAbilityEnded.AddUObject(this, &UBTTask_ActivateAbility::OnAbilityEnded);
	
	if (!AbilitySystem->TryActivateAbilitiesByTag(TagContainer))
	{
		return EBTNodeResult::Failed;
	}
	
	return EBTNodeResult::InProgress;
}

void UBTTask_ActivateAbility::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
                                             EBTNodeResult::Type TaskResult)
{
	if (AbilitySystem.IsValid() && AbilityEndedHandle.IsValid())
	{
		AbilitySystem->OnAbilityEnded.Remove(AbilityEndedHandle);
	}
	
	AbilitySystem.Reset();
	BehaviourTree.Reset();
	AbilityEndedHandle.Reset();
	ActiveAbilityTag = FGameplayTag();

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void UBTTask_ActivateAbility::OnAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (!AbilityEndedData.AbilityThatEnded) return;
	if (!AbilityEndedData.AbilityThatEnded->AbilityTags.HasTag(ActiveAbilityTag)) return;

	
	if (BehaviourTree.IsValid())
	{
		// 적이 파괴되는 경우 Fail
		EBTNodeResult::Type Result = AbilityEndedData.bWasCancelled ? EBTNodeResult::Failed : EBTNodeResult::Succeeded;

		FinishLatentTask(*BehaviourTree.Get(), Result);
	}
}

FString UBTTask_ActivateAbility::GetStaticDescription() const
{
	if (AbilityTag.IsValid())
	{
		return FString::Printf(TEXT("Activate Ability\n%s"), *AbilityTag.ToString());
	}
	return TEXT("Activate Ability\n[From Blackboard]");
}
