// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/BTTask_SetRandomAttackTag.h"

#include "AIController.h"
#include "HOGDebugHelper.h"
#include "Character/Enemy/Interface/IMeleeAttacker.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_SetRandomAttackTag::UBTTask_SetRandomAttackTag()
{
	NodeName = "Set Random Attack Tag";
	
	AbilityTagKey.AddNameFilter(this,                                    
	GET_MEMBER_NAME_CHECKED(UBTTask_SetRandomAttackTag, AbilityTagKey));
}

EBTNodeResult::Type UBTTask_SetRandomAttackTag::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard) return EBTNodeResult::Failed;

	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	IIMeleeAttacker* Attacker = Cast<IIMeleeAttacker>(AIC->GetPawn());
	if (!Attacker) return EBTNodeResult::Failed;

	// UDA_MeleeEnemyConfig* Config = Cast<UDA_MeleeEnemyConfig>(Goblin->GetEnemyConfig());
	// if (!Config || Config->MeleeAttacks.IsEmpty()) return EBTNodeResult::Failed;
	
	TArray<FGameplayTag> Tags = Attacker->GetMeleeAttackTags();
	if (Tags.IsEmpty()) return EBTNodeResult::Failed;
	
	// Config의 MeleeAttacks에서 랜덤 선택
	int32 RandomIndex = FMath::RandRange(0, Tags.Num() - 1);
	
	//const FGoblinAttackData& Selected = Config->MeleeAttacks[RandomIndex];
	
	Blackboard->SetValueAsName(AbilityTagKey.SelectedKeyName, Tags[RandomIndex].GetTagName());

	return EBTNodeResult::Succeeded;
	
}

void UBTTask_SetRandomAttackTag::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		AbilityTagKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTTask_SetRandomAttackTag::GetStaticDescription() const
{
	return FString::Printf(TEXT("Set Goblin Attack Type\nKey: %s"),
		*AbilityTagKey.SelectedKeyName.ToString());

}
