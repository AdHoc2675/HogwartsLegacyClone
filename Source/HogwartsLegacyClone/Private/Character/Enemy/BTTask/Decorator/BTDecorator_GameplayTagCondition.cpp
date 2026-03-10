// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/Decorator/BTDecorator_GameplayTagCondition.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AIController.h"

UBTDecorator_GameplayTagCondition::UBTDecorator_GameplayTagCondition()
{
	NodeName = "Has Gameplay Tag";
	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
	bCreateNodeInstance = true;
	FlowAbortMode = EBTFlowAbortMode::Both;
}

// ex) 현재 캐릭터(적)이 분노 상태인 경우, 조건을 충족하여 시퀀스 조건 충족
// ex) 현재 캐릭터(적)이 스턴상태인경우 inverse로 설정하여 시퀀스 불충족
bool UBTDecorator_GameplayTagCondition::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	if (!AbilityComponent.IsValid()) return false;

	const bool bHasTag = AbilityComponent->HasMatchingGameplayTag(TagToCheck);
	return IsInversed() ? !bHasTag : bHasTag;
}

FString UBTDecorator_GameplayTagCondition::GetStaticDescription() const
{
	FString Prefix = IsInversed() ? TEXT("NOT ") : TEXT("");
	return FString::Printf(TEXT("%sHas Tag: %s"), *Prefix, *TagToCheck.ToString());
}

void UBTDecorator_GameplayTagCondition::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);
	
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return;

	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(AIController->GetPawn());
	if (!ASCInterface) return;

	AbilityComponent = ASCInterface->GetAbilitySystemComponent();
	if (!AbilityComponent.IsValid()) return;

	// 태그 변경 감지
	TagChangedHandle = AbilityComponent->RegisterGameplayTagEvent(TagToCheck, EGameplayTagEventType::NewOrRemoved)
		.AddLambda([this, &OwnerComp](const FGameplayTag& Tag, int32 NewCount)
		{
			OwnerComp.RequestExecution(this);
		});
}

void UBTDecorator_GameplayTagCondition::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
	
	if (AbilityComponent.IsValid())
	{
		AbilityComponent->RegisterGameplayTagEvent(TagToCheck, EGameplayTagEventType::NewOrRemoved)
			.Remove(TagChangedHandle);
	}

	TagChangedHandle.Reset();
	AbilityComponent.Reset();
}
