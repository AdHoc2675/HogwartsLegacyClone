// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/Service/BTService_UpdateTargetInfo.h"

#include "AIController.h"
#include "HOGDebugHelper.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/Enemy/Helper/AIBlackboardHelper.h"
#include "Character/Enemy/Helper/DistanceCalculator.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

UBTService_UpdateTargetInfo::UBTService_UpdateTargetInfo()
{
	NodeName = "Update Target Info";
	Interval = 0.2f;
	RandomDeviation = 0.05f;
}

void UBTService_UpdateTargetInfo::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;
	
	AAIController* Controller = OwnerComp.GetAIOwner();
	if (!Controller) return;
	
	APawn* Pawn = Controller->GetPawn();
	if (!Pawn) return;
	
	AActor* Target = UAIBlackboardHelper::GetTargetActor(BB);
	if (!Target)
	{
		UAIBlackboardHelper::SetTargetDistance(BB, MAX_FLT);
		return;
	}
	
	float Distance = UDistanceCalculator::Calculate(Pawn, Target);
	UAIBlackboardHelper::SetTargetDistance(BB, Distance);
	
}

