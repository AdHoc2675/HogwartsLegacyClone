// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/Decorator/BTDecorator_IsInMeleeRange.h"
#include "Character/Enemy/Interface/IMeleeAttacker.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UBTDecorator_IsInMeleeRange::UBTDecorator_IsInMeleeRange()
{
	NodeName = "Is In Melee Range";
	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
	FlowAbortMode = EBTFlowAbortMode::Both;

	TargetDistanceKey.AddFloatFilter(this,
	                                 GET_MEMBER_NAME_CHECKED(UBTDecorator_IsInMeleeRange, TargetDistanceKey));
}

bool UBTDecorator_IsInMeleeRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return false;

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn) return false;

	IIMeleeAttacker* MeleeAttacker = Cast<IIMeleeAttacker>(Pawn);
	if (!MeleeAttacker) return false;

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	float Distance = Blackboard->GetValueAsFloat(TargetDistanceKey.SelectedKeyName);

	// 캐릭터의 근접 공격 사거리
	float Range = MeleeAttacker->GetMeleeAttackRange();

	// 타겟까지의 거리가 공격 범위 이내인지 체크
	return Distance <= Range;
}

void UBTDecorator_IsInMeleeRange::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetDistanceKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTDecorator_IsInMeleeRange::GetStaticDescription() const
{
	return Super::GetStaticDescription();
}

void UBTDecorator_IsInMeleeRange::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard) return;

	// 혹시 이전 Observer가 남아있을 경우를 대비해 먼저 정리
	Blackboard->UnregisterObserversFrom(this);

	const bool bInitialResult = CalculateRawConditionValue(OwnerComp, NodeMemory);

	// TargetDistance 키 값이 변경될 때마다 콜백 호출
	Blackboard->RegisterObserver(
		TargetDistanceKey.GetSelectedKeyID(),
		this,
		FOnBlackboardChangeNotification::CreateLambda(
			// bLastResult: 이전 조건 결과를 람다가 캡처해서 변경 여부 비교
			[this, bLastResult = bInitialResult](const UBlackboardComponent& BlackboardComp,
			                                     FBlackboard::FKey ChangedKeyID) mutable
			{
				UBehaviorTreeComponent* BehaviourTree = Cast<
					UBehaviorTreeComponent>(BlackboardComp.GetBrainComponent());
				if (!BehaviourTree) return EBlackboardNotificationResult::ContinueObserving;

				const bool bCurrentResult = CalculateRawConditionValue(*BehaviourTree, nullptr);

				// 이전 결과와 현재 결과가 다를 때만 BT 재평가 요청
				// (매번 RequestExecution 호출 방지)	
				if (bCurrentResult != bLastResult)
				{
					bLastResult = bCurrentResult;
					BehaviourTree->RequestExecution(this);
				}
				// Observer 유지
				return EBlackboardNotificationResult::ContinueObserving;
			}
		)
	);
}

void UBTDecorator_IsInMeleeRange::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (Blackboard)
	{
		Blackboard->UnregisterObserversFrom(this);
	}
}
