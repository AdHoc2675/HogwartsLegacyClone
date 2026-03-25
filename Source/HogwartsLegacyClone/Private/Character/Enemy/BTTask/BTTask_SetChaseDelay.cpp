// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/BTTask_SetChaseDelay.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Character/Enemy/Helper/AIBlackboardHelper.h"

UBTTask_SetChaseDelay::UBTTask_SetChaseDelay()
{
	NodeName = "Set Chase Delay";
}

EBTNodeResult::Type UBTTask_SetChaseDelay::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;
	
	UAIBlackboardHelper::SetChaseDelay(BB, true);
	
	TWeakObjectPtr<UBlackboardComponent> WeakBB = BB;
	
	UWorld* World = OwnerComp.GetWorld();
	
	World->GetTimerManager().ClearTimer(TimerHandle);
	
	// 딜레이 시간이 지난 후 
	World->GetTimerManager().SetTimer(
		TimerHandle,
		[WeakBB]
		{
			if (WeakBB.IsValid())
			{
				UAIBlackboardHelper::SetChaseDelay(WeakBB.Get(), false);
			}
		}, DelayTime, false);
	
	return EBTNodeResult::Succeeded;
}

FString UBTTask_SetChaseDelay::GetStaticDescription() const
{
	return FString::Printf(TEXT("Set Chase Delay\n%.1fs"), DelayTime);
}

void UBTTask_SetChaseDelay::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTNodeResult::Type TaskResult)
{
	// 우선권이 넘어간 경우
	if (TaskResult == EBTNodeResult::Aborted)
	{
		OwnerComp.GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
		
		if (UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent())
		{
			UAIBlackboardHelper::SetChaseDelay(BB, false);
		}
	}
	
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
