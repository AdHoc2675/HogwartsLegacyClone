// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/Service/BTService_UpdateTargetInfo.h"

#include "AIController.h"
#include "HOGDebugHelper.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

UBTService_UpdateTargetInfo::UBTService_UpdateTargetInfo()
{
	NodeName = "Update Target Info";
	
	// 0.2초마다 Tick
	Interval = 0.2f;
	RandomDeviation = 0.05f;
	
	bNotifyBecomeRelevant = true;
	
	TargetActorKey.AddObjectFilter(this,
	GET_MEMBER_NAME_CHECKED(UBTService_UpdateTargetInfo, TargetActorKey),
	AActor::StaticClass());

	TargetDistanceKey.AddFloatFilter(this,
		GET_MEMBER_NAME_CHECKED(UBTService_UpdateTargetInfo, TargetDistanceKey));
}

void UBTService_UpdateTargetInfo::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC)
	{
		UE_LOG(LogTemp, Warning, TEXT("no AIC"));
		return;
	}
    
	APawn* CurrentPawn = AIC->GetPawn();
	if (!CurrentPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("no CurrentPawn"));
		return;
	}
    
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("no Blackboard"));
		return;
	}
	
	const AActor* TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName));
    
	// Sensing 범위를 벗어난 경우
	if (!TargetActor)
	{
		Blackboard->SetValueAsFloat(TargetDistanceKey.SelectedKeyName, MAX_FLT);
		return;
	}
    
	// 두 객체의 거리 계산
	float Distance = FVector::Distance(TargetActor->GetActorLocation(), CurrentPawn->GetActorLocation());
	
	// 객체의 캡슐컴포넌트의 Radius는 거리계산에서 제외
	if (const ACharacter* PawnChar = Cast<ACharacter>(CurrentPawn))
	{
		Distance -= PawnChar->GetCapsuleComponent()->GetScaledCapsuleRadius();
	}
	
	if (const ACharacter* TargetChar = Cast<ACharacter>(TargetActor))
	{
		Distance -= TargetChar->GetCapsuleComponent()->GetScaledCapsuleRadius();
	}
	Distance = FMath::Max(0.f, Distance);
	
	// 두 객체 사이의 거리 최신화
	Blackboard->SetValueAsFloat(TargetDistanceKey.SelectedKeyName, Distance);
}

void UBTService_UpdateTargetInfo::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	
	// 키 연결
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
		TargetDistanceKey.ResolveSelectedKey(*BBAsset);
	}
}

