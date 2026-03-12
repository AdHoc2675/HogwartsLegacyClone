// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/BTTask_SetGoblinAttackType.h"

#include "AIController.h"
#include "HOGDebugHelper.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/Enemy/GoblinEnemyCharacter.h"
#include "Data/Enemy/DA_GoblinConfig.h"

UBTTask_SetGoblinAttackType::UBTTask_SetGoblinAttackType()
{
	NodeName = "Set Goblin Attack Type";
	
	AbilityTagKey.AddNameFilter(this,                                    
	GET_MEMBER_NAME_CHECKED(UBTTask_SetGoblinAttackType, AbilityTagKey));
}

EBTNodeResult::Type UBTTask_SetGoblinAttackType::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// if (AttackTypes.IsEmpty()) return EBTNodeResult::Failed;
	//
	// UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	// if (!Blackboard) return EBTNodeResult::Failed;
	//
	// int32 RandomIndex = FMath::RandRange(0, AttackTypes.Num() - 1);
	// Blackboard->SetValueAsInt(AttackTypeKey.SelectedKeyName, static_cast<int32>(AttackTypes[RandomIndex]));
	//
	// return EBTNodeResult::Succeeded;
	
	
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard) return EBTNodeResult::Failed;

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	AGoblinEnemyCharacter* Goblin = Cast<AGoblinEnemyCharacter>(AIC->GetPawn());
	if (!Goblin) return EBTNodeResult::Failed;

	UDA_GoblinConfig* Config = Cast<UDA_GoblinConfig>(Goblin->GetEnemyConfig());
	if (!Config || Config->MeleeAttacks.IsEmpty()) return EBTNodeResult::Failed;

	// Config의 MeleeAttacks에서 랜덤 선택
	int32 RandomIndex = FMath::RandRange(0, Config->MeleeAttacks.Num() - 1);
	
	const FGoblinAttackData& Selected = Config->MeleeAttacks[RandomIndex];
	
	Blackboard->SetValueAsName(AbilityTagKey.SelectedKeyName, Selected.AbilityTag.GetTagName());

	return EBTNodeResult::Succeeded;
	
}

void UBTTask_SetGoblinAttackType::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		AbilityTagKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTTask_SetGoblinAttackType::GetStaticDescription() const
{
	return FString::Printf(TEXT("Set Goblin Attack Type\nKey: %s"),
		*AbilityTagKey.SelectedKeyName.ToString());

}
