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
	bCreateNodeInstance = true;

	TargetDistanceKey.AddFloatFilter(this,
	                                 GET_MEMBER_NAME_CHECKED(UBTDecorator_IsInMeleeRange, TargetDistanceKey));
}

bool UBTDecorator_IsInMeleeRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC)
	{
		UE_LOG(LogTemp, Warning, TEXT("no AIC"));
	}

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("no Pawn"));
		return false;
	}

	IIMeleeAttacker* MeleeAttacker = Cast<IIMeleeAttacker>(Pawn);
	if (!MeleeAttacker)
	{
		UE_LOG(LogTemp, Warning, TEXT("no MeleeAttacker"));
		return false;
	}

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("no Blackboard"));
		return false;
	}

	float Distance = Blackboard->GetValueAsFloat(TargetDistanceKey.SelectedKeyName);
	
	// 직접 태그가 있으면 그걸 사용, 없으면 BB에서 읽기
	FName AttackTag;
	if (DirectAttackTag.IsValid())
	{
		AttackTag = DirectAttackTag.GetTagName();
		//UE_LOG(LogTemp, Warning, TEXT("Check Point : %s"), *AttackTag.ToString());
	}
	else
	{
		AttackTag = Blackboard->GetValueAsName(AttackTagKey.SelectedKeyName);
	}

	float MinRange, MaxRange;
	MeleeAttacker->GetMeleeAttackRange(AttackTag, MinRange, MaxRange);
	
	return Distance >= MinRange && Distance <= MaxRange;
	
}

void UBTDecorator_IsInMeleeRange::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetDistanceKey.ResolveSelectedKey(*BBAsset);
		AttackTagKey.ResolveSelectedKey(*BBAsset);
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

	if (UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent())
	{
		Blackboard->UnregisterObserversFrom(this);
	}
}
