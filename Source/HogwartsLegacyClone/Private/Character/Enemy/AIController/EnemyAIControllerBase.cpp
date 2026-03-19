// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Enemy/AIController/EnemyAIControllerBase.h"

#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/Enemy/EnemyCharacterBase.h"
#include "Core/HOG_GameplayTags.h"
#include "Perception/AIPerceptionComponent.h"

#include "Perception/AIPerceptionTypes.h"

void AEnemyAIControllerBase::ActivateAI()
{
	if (bActivated) return;
	bActivated = true;
	
	StartBehaviorTree();
	BehaviorTreeComp = Cast<UBehaviorTreeComponent>(BrainComponent);
	BlackboardComp = GetBlackboardComponent();
}

AEnemyAIControllerBase::AEnemyAIControllerBase()
{
	// Perception 컴포넌트 생성
	UAIPerceptionComponent* AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>("AIPerceptionComp");
	SetPerceptionComponent(*AIPerceptionComp);
}

AActor* AEnemyAIControllerBase::GetTargetActor() const
{
	return BlackboardComp ? Cast<AActor>(BlackboardComp->GetValueAsObject(BB_TargetActor)) : nullptr;
}

void AEnemyAIControllerBase::SetTargetActor(AActor* TargetActor)
{
	if (BlackboardComp)
	{
		BlackboardComp->SetValueAsObject(BB_TargetActor, TargetActor);
	}
}

void AEnemyAIControllerBase::ClearTargetActor()
{
	if (BlackboardComp)
	{
		BlackboardComp->ClearValue(BB_TargetActor);
	}
}

AEnemyCharacterBase* AEnemyAIControllerBase::GetEnemyCharacter() const
{
	return EnemyCharacter;
}

void AEnemyAIControllerBase::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
    
	EnemyCharacter = Cast<AEnemyCharacterBase>(InPawn);
	if (!EnemyCharacter) return;
	
	if (EnemyCharacter->GetActivationMode() == EAIActivationMode::Immediate)
	{
		ActivateAI();
	}
	
	// 콜백 등록은 마지막에
	BindCallbacks();
}

void AEnemyAIControllerBase::OnUnPossess()
{
	StopBehaviorTree();
	
	BehaviorTreeComp = nullptr;
	BlackboardComp = nullptr;
	EnemyCharacter = nullptr;
	
	Super::OnUnPossess();
}

void AEnemyAIControllerBase::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!bActivated) return; 
	if (!Actor) return;
	
	if (Stimulus.WasSuccessfullySensed())
	{
		// 이미 타겟이면 무시
		if (GetTargetActor() == Actor) return;
		
		// 새로운 타겟일 때 검증
		ABaseCharacter* PerceivedCharacter = Cast<ABaseCharacter>(Actor);
		if (!PerceivedCharacter) return;
		
		// 같은 팀인지 검증
		if (!PerceivedCharacter->HasTeamTag(HOGGameplayTags::Team_Player)) return;
		
		SetTargetActor(Actor);
		
		return;
	}
	
	//현재 타겟을 놏쳤을 때 타겟 클리어
	if (GetTargetActor() == Actor)
	{
		ClearTargetActor();
	}
}

void AEnemyAIControllerBase::OnEnemyDeath()
{
	StopBehaviorTree();
	ClearTargetActor();
}

void AEnemyAIControllerBase::StartBehaviorTree()
{
	AEnemyCharacterBase* Enemy = GetEnemyCharacter();
	if (!Enemy) return;
	
	UBehaviorTree* BehaviorTree = Enemy->GetBehaviorTree();
	if (!BehaviorTree) return;
	
	RunBehaviorTree(BehaviorTree);
}

void AEnemyAIControllerBase::StopBehaviorTree()
{
	if (BehaviorTreeComp)
	{
		BehaviorTreeComp->StopTree();
	}
}

// 콜백 함수 등록
void AEnemyAIControllerBase::BindCallbacks()
{
	// OnTargetPerceptionUpdated 콜백 함수 등록
	if (UAIPerceptionComponent* AIPerceptionComp = GetPerceptionComponent())
	{
		AIPerceptionComp->OnTargetPerceptionUpdated.
		                  AddDynamic(this, &AEnemyAIControllerBase::OnTargetPerceptionUpdated);
	}
	
	// Death 콜백 함수 등록
	if (AEnemyCharacterBase* Enemy = GetEnemyCharacter())
	{
		Enemy->OnEnemyDeath.AddUObject(this, &AEnemyAIControllerBase::OnEnemyDeath);
	}
}
