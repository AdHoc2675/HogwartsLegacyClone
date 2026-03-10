// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/Decorator/BTDecorator_IsInDashRange.h"
#include "Character/Enemy/Interface/IDashable.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UBTDecorator_IsInDashRange::UBTDecorator_IsInDashRange()
{
	NodeName = "Is In Dash Range";
}

bool UBTDecorator_IsInDashRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return false;

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn) return false;

	IIDashable* Dashable = Cast<IIDashable>(Pawn);
	if (!Dashable) return false;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
    
	// Service가 계산해둔 거리 사용
	float Distance = BB->GetValueAsFloat("TargetDistance");
	float MinRange = Dashable->GetDashMinRange();
	float MaxRange = Dashable->GetDashMaxRange();

	return Distance >= MinRange && Distance <= MaxRange;
}

void UBTDecorator_IsInDashRange::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTDecorator_IsInDashRange::GetStaticDescription() const
{
	return TEXT("Is In Dash Range (IDashable)");
}
