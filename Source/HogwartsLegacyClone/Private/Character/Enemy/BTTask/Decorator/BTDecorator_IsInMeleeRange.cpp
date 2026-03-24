// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/Decorator/BTDecorator_IsInMeleeRange.h"
#include "Character/Enemy/Interface/IMeleeAttacker.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Character/Enemy/Helper/AIBlackboardHelper.h"
#include "Character/Enemy/Helper/AttackInfoProvider.h"

UBTDecorator_IsInMeleeRange::UBTDecorator_IsInMeleeRange()
{
	NodeName = "Is In Melee Range";
	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
	FlowAbortMode = EBTFlowAbortMode::LowerPriority;
	bCreateNodeInstance = true;
}

bool UBTDecorator_IsInMeleeRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return false;

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return false;

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn) return false;

	float Distance = UAIBlackboardHelper::GetTargetDistance(BB);

	FName AttackTag = DirectAttackTag.IsValid()
		                  ? DirectAttackTag.GetTagName()
		                  : UAIBlackboardHelper::GetAbilityTagName(BB);

	float MinRange, MaxRange;
	if (!UAttackInfoProvider::GetRange(Pawn, AttackTag, MinRange, MaxRange)) return false;

	return Distance >= MinRange && Distance <= MaxRange;
}

FString UBTDecorator_IsInMeleeRange::GetStaticDescription() const
{
	if (DirectAttackTag.IsValid())
	{
		return FString::Printf(TEXT("In Range: %s"), *DirectAttackTag.ToString());
	}
	return TEXT("Is In Melee Range");
}

void UBTDecorator_IsInMeleeRange::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	//혹시 이전 Observer가 남아있을 경우를 대비해 먼저 정리
	BB->UnregisterObserversFrom(this);

	const bool bInitialResult = CalculateRawConditionValue(OwnerComp, NodeMemory);

	BB->RegisterObserver(
		BB->GetKeyID("TargetDistance"),
		this,
		FOnBlackboardChangeNotification::CreateLambda(
			[this, &OwnerComp, bLastResult = bInitialResult](
			const UBlackboardComponent& BlackboardComp, FBlackboard::FKey ChangedKey) mutable
			{
				UBehaviorTreeComponent* BT = Cast<UBehaviorTreeComponent>(BlackboardComp.GetBrainComponent());

				if (!BT) return EBlackboardNotificationResult::ContinueObserving;

				const bool bCurrentResult = CalculateRawConditionValue(*BT, nullptr);

				// 이전 결과와 현재 결과가 다를 때만 BT 재평가 요청
				// 				// (매번 RequestExecution 호출 방지)	
				if (bCurrentResult != bLastResult)
				{
					bLastResult = bCurrentResult;
					BT->RequestExecution(this);
				}
				return EBlackboardNotificationResult::ContinueObserving;
			}
		)
	);
}

void UBTDecorator_IsInMeleeRange::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent())
	{
		Blackboard->UnregisterObserversFrom(this);
	}
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}
