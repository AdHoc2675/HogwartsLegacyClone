// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/BTTask_MoveToTarget.h"

#include "AIController.h"
#include "HOGDebugHelper.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "Character/Enemy/Helper/AIBlackboardHelper.h"
#include "Character/Enemy/Helper/AttackInfoProvider.h"
#include "Character/Enemy/Interface/IMeleeAttacker.h"

UBTTask_MoveToTarget::UBTTask_MoveToTarget()
{
	NodeName = "Move To Target";
	bNotifyTick = true;
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_MoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AIController = OwnerComp.GetAIOwner();
	if (!AIController.IsValid()) return EBTNodeResult::Failed;
	
	APawn* Pawn = AIController->GetPawn();
	if (!Pawn) return EBTNodeResult::Failed;
	
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;
	
	AActor* Target = UAIBlackboardHelper::GetTargetActor(BB);
	if (!Target) return EBTNodeResult::Failed;
	
	// 공격 범위
	float AcceptanceRadius = GetAcceptanceRadius(BB, Pawn);
	
	// 이동 결괄
	EPathFollowingRequestResult::Type Result = AIController->MoveToActor(Target, AcceptanceRadius, true, true,true,nullptr, true);
	
	// 경로가 없는 경우
	if (Result == EPathFollowingRequestResult::Failed) return EBTNodeResult::Failed;
	
	// 도착
	if (Result == EPathFollowingRequestResult::AlreadyAtGoal) return EBTNodeResult::Succeeded;
	
	// 이동 중
	return EBTNodeResult::InProgress;
}

void UBTTask_MoveToTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (!AIController.IsValid())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	AActor* Target = UAIBlackboardHelper::GetTargetActor(BB);
	if (!Target)
	{
		AIController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	float Distance = UAIBlackboardHelper::GetTargetDistance(BB);
	float AcceptanceRadius = GetAcceptanceRadius(BB, AIController->GetPawn());
	
	// 도착 시 완료
	if (Distance <= AcceptanceRadius)
	{
		AIController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
	
	// 길이 있고
	if (UPathFollowingComponent* PathComp = AIController->GetPathFollowingComponent())
	{
		// 움직이지 않을 떄
		if (PathComp->GetStatus() != EPathFollowingStatus::Moving)
		{
			// 이동
			AIController->MoveToActor(Target, AcceptanceRadius, true, true,true,nullptr, true);
		}
	}
}

void UBTTask_MoveToTarget::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTNodeResult::Type TaskResult)
{
	// Task 노드의 우선권이 변경
	if (AIController.IsValid() && TaskResult != EBTNodeResult::Aborted)
	{
		AIController->StopMovement();
	}
	AIController->Reset();
}

float UBTTask_MoveToTarget::GetAcceptanceRadius(UBlackboardComponent* BB, APawn* Pawn) const
{
	if (!BB || !Pawn) return 150.f;
	
	FName AttackTag = UAIBlackboardHelper::GetAbilityTagName(BB);
	
	float MinRange, MaxRange;
	if (UAttackInfoProvider::GetRange(Pawn, AttackTag, MinRange, MaxRange))
	{
		return MaxRange;
	}
	return 150.f;
}
