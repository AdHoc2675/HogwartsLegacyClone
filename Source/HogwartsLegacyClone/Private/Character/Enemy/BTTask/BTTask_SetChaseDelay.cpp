// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/BTTask_SetChaseDelay.h"

#include "BehaviorTree/BlackboardComponent.h"

UBTTask_SetChaseDelay::UBTTask_SetChaseDelay()
{
	NodeName = "Set Chase Delay";
	ChaseDelayKey.AddBoolFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_SetChaseDelay, ChaseDelayKey));
}

EBTNodeResult::Type UBTTask_SetChaseDelay::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard) return EBTNodeResult::Failed;
	
	Blackboard->SetValueAsBool(ChaseDelayKey.SelectedKeyName, true);
    
	TWeakObjectPtr<UBlackboardComponent> WeakBlackboard = Blackboard;
	FName KeyName = ChaseDelayKey.SelectedKeyName;
	float Time = DelayTime;
	
	OwnerComp.GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
	
	// 플레이어가 적의 타격 범위를 벗아난경우 다시 쫓아가기전 기다리는 타이머
	OwnerComp.GetWorld()->GetTimerManager().SetTimer(
	   TimerHandle,
	   [WeakBlackboard, KeyName, Time]()
	   {
		  if (WeakBlackboard.IsValid())
		  {
			 WeakBlackboard->SetValueAsBool(KeyName, false);
		  }
	   },
	   DelayTime,
	   false
	);
    
	return EBTNodeResult::Succeeded;
}

void UBTTask_SetChaseDelay::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
    
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		ChaseDelayKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTTask_SetChaseDelay::GetStaticDescription() const
{
	if (!ChaseDelayKey.SelectedKeyName.IsValid())
	{
		return TEXT("Set Chase Delay\n[No Key Set]");
	}

	return FString::Printf(TEXT("Set Chase Delay\nDelay: %.1fs"),
		DelayTime);

}

void UBTTask_SetChaseDelay::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTNodeResult::Type TaskResult)
{
	// 적이 파괴되는 경우
	if (TaskResult == EBTNodeResult::Aborted)
	{
		OwnerComp.GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
		
		if (UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent())
		{
			Blackboard->SetValueAsBool(ChaseDelayKey.SelectedKeyName, false);
		}
	}
	
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
