// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/BTTask_ActivateAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_ActivateAbility::UBTTask_ActivateAbility()
{
	NodeName = "Activate Ability";
	bNotifyTaskFinished = true;
	bCreateNodeInstance = true;

	AbilityTagKey.AddNameFilter(this,
	                            GET_MEMBER_NAME_CHECKED(UBTTask_ActivateAbility, AbilityTagKey));

	TargetActorKey.AddObjectFilter(this,
	                               GET_MEMBER_NAME_CHECKED(UBTTask_ActivateAbility, TargetActorKey),
	                               AActor::StaticClass());
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

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	
	// 태그를 직접 설정했으면 설정된 태그로 어빌리티 실행
	ActiveAbilityTag = AbilityTag;

	// 태그 설정이 안되어 있고 AbilityTagKey가 유효하면 Blackboard에서 읽기
	if (!ActiveAbilityTag.IsValid() && Blackboard && AbilityTagKey.SelectedKeyName.IsValid())
	{
		FName TagName = Blackboard->GetValueAsName(AbilityTagKey.SelectedKeyName);
		if (!TagName.IsNone())
		{
			ActiveAbilityTag = FGameplayTag::RequestGameplayTag(TagName);
		}
	}

	if (!ActiveAbilityTag.IsValid()) return EBTNodeResult::Failed;

	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(ActiveAbilityTag);

	BehaviourTree = &OwnerComp;
	AbilityEndedHandle = AbilitySystem->OnAbilityEnded.AddUObject(this, &UBTTask_ActivateAbility::OnAbilityEnded);

	bool bSuccess = AbilitySystem->TryActivateAbilitiesByTag(TagContainer);
	if (!bSuccess) return EBTNodeResult::Failed;
	
	return EBTNodeResult::InProgress;
}

void UBTTask_ActivateAbility::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
                                             EBTNodeResult::Type TaskResult)
{
	
	if (AbilitySystem.IsValid())
	{
		AbilitySystem->OnAbilityEnded.Remove(AbilityEndedHandle);
		AbilitySystem.Reset();
	}

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
	if (!AbilityTag.IsValid())
	{
		return TEXT("Activate Ability\n[No Tag Set]");
	}

	return FString::Printf(TEXT("Activate Ability\nTag: %s"), *AbilityTag.ToString());
}

void UBTTask_ActivateAbility::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		AbilityTagKey.ResolveSelectedKey(*BBAsset);
		TargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}
