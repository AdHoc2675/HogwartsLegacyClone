// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/BTTask/BTTask_ActivateAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AIController.h"

UBTTask_ActivateAbility::UBTTask_ActivateAbility()
{
	NodeName = "Activate Ability";
}

EBTNodeResult::Type UBTTask_ActivateAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController) return EBTNodeResult::Failed;
	
	APawn* Pawn = AIController->GetPawn();
	if (!Pawn) return EBTNodeResult::Failed;
	
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Pawn);
	if (!ASCInterface) return EBTNodeResult::Failed;
	
	UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent();
	if (!ASC) return EBTNodeResult::Failed;
	
	// 실행할 어빌리티 추가
	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(AbilityTag);
	
	// 어빌리티 실행
	bool bSuccess = ASC->TryActivateAbilitiesByTag(TagContainer);
	
	return bSuccess ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

FString UBTTask_ActivateAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("Activate: %s"), *AbilityTag.ToString());
}
