// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/BTTask_MoveToTarget.h"

#include "AIController.h"
#include "HOGDebugHelper.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Character/Enemy/EnemyCharacterBase.h"

UBTTask_MoveToTarget::UBTTask_MoveToTarget()
{
	NodeName = "Move To Target";
	bNotifyTick = true;
    
	// 인스턴스화 하지 않으면 Task를 공유하는 문제
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_MoveToTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AIController = OwnerComp.GetAIOwner();
	if (!AIController.IsValid()) return EBTNodeResult::Failed;
	
	Pawn = AIController->GetPawn();
	if (!Pawn.IsValid()) return EBTNodeResult::Failed;;
	
	Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard.IsValid())  return EBTNodeResult::Failed;
	
	Enemy = Cast<AEnemyCharacterBase>(Pawn.Get());
	if (!Enemy.IsValid()) return EBTNodeResult::Failed;
	
	AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!TargetActor) return EBTNodeResult::Failed;
	
	// 공격범위
	float AttackRange = Enemy->GetMinAttackRange();
	
	EPathFollowingRequestResult::Type MoveResult = AIController->MoveToActor(
		TargetActor, AttackRange, true, true, true, nullptr, true);
	
	// 경로가 없는 경우
	if (MoveResult == EPathFollowingRequestResult::Failed)
		return EBTNodeResult::Failed;
	
	// 이미 Range 안에 있는 경우
	if (MoveResult == EPathFollowingRequestResult::AlreadyAtGoal)
		return EBTNodeResult::Succeeded;
	
	return EBTNodeResult::InProgress;
	
}

void UBTTask_MoveToTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (!AIController.IsValid() || !Blackboard.IsValid() || !Enemy.IsValid())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!TargetActor)
	{
		AIController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	
	float Distance = Blackboard->GetValueAsFloat(TargetDistanceKey.SelectedKeyName);
	float AttackRange = Enemy->GetMinAttackRange();
	
	// 공격 범위 이내 인 경우 Success
	if (Distance <= AttackRange)
	{
		AIController->StopMovement();
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
	
	// 경로 갱신 (타겟 이동 시)
	UPathFollowingComponent* PathComp = AIController->GetPathFollowingComponent();
	if (PathComp && PathComp->GetStatus() != EPathFollowingStatus::Moving)
	{
		AIController->MoveToActor(TargetActor, AttackRange, true, true, true, nullptr, true);
	}
}

void UBTTask_MoveToTarget::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTNodeResult::Type TaskResult)
{
	// Abort 케이스인경우 정지
	if (AIController.IsValid() && TaskResult != EBTNodeResult::Aborted)
	{
		AIController->StopMovement();
	}
	
	AIController.Reset();
	Pawn.Reset();
	Blackboard.Reset();
	Enemy.Reset();
}

void UBTTask_MoveToTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
		TargetDistanceKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTTask_MoveToTarget::GetStaticDescription() const
{
	if (!TargetActorKey.SelectedKeyName.IsValid() || !TargetDistanceKey.SelectedKeyName.IsValid())
	{
		return TEXT("Move To Target\n[No Key Set]");
	}

	return FString::Printf(TEXT("Move To Target\nTarget: %s\nDistance Key: %s"),
		*TargetActorKey.SelectedKeyName.ToString(),
		*TargetDistanceKey.SelectedKeyName.ToString());
}
