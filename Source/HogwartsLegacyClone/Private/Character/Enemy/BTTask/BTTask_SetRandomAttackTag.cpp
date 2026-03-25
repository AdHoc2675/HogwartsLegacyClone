// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/BTTask_SetRandomAttackTag.h"

#include "AIController.h"
#include "HOGDebugHelper.h"
#include "Character/Enemy/Interface/IMeleeAttacker.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/Enemy/Helper/AIBlackboardHelper.h"
#include "Character/Enemy/Helper/AttackInfoProvider.h"

UBTTask_SetRandomAttackTag::UBTTask_SetRandomAttackTag()
{
	NodeName = "Set Random Attack Tag";
}

EBTNodeResult::Type UBTTask_SetRandomAttackTag::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;
	
	APawn* Pawn = AIC->GetPawn();
	if (!Pawn) return EBTNodeResult::Failed;
	
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return EBTNodeResult::Failed;
	
	TArray<FGameplayTag> Tags = UAttackInfoProvider::GetAllTags(Pawn);
	if (Tags.IsEmpty()) return EBTNodeResult::Failed;
	
	// 제외 태그 
	Tags.RemoveAll([this](const FGameplayTag& Tag)
	{
		return ExcludeTags.HasTag(Tag);
	});
	
	if (Tags.IsEmpty()) return EBTNodeResult::Failed;
	
	int32 RandomIndex = FMath::RandRange(0, Tags.Num() - 1);
	
	// 랜덤한 어빌리티 set
	UAIBlackboardHelper::SetAbilityTagName(BB, Tags[RandomIndex].GetTagName());
	
	return EBTNodeResult::Succeeded;
}
FString UBTTask_SetRandomAttackTag::GetStaticDescription() const
{
	return TEXT("Set Random Attack Tag");
}
