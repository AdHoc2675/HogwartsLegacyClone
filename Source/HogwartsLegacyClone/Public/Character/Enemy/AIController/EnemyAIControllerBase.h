// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIControllerBase.generated.h"

/**
 * 
 */
struct FAIStimulus;
class UBehaviorTreeComponent;
class UBlackboardComponent;
class AEnemyCharacterBase;

UCLASS()
class HOGWARTSLEGACYCLONE_API AEnemyAIControllerBase : public AAIController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "AI")
	void ActivateAI();
	
	AEnemyAIControllerBase();
	
	AActor* GetTargetActor() const;
	void SetTargetActor(AActor* TargetActor);
	void ClearTargetActor();
	
	AEnemyCharacterBase* GetEnemyCharacter() const;

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void OnEnemyDeath();

	UPROPERTY()
	UBehaviorTreeComponent* BehaviorTreeComp;

	UPROPERTY()
	UBlackboardComponent* BlackboardComp;
	
	UPROPERTY()
	AEnemyCharacterBase* EnemyCharacter;

	bool bActivated = false;

private:
	void StartBehaviorTree();
	void StopBehaviorTree();
	void BindCallbacks();
	
	inline static const FName BB_TargetActor = TEXT("TargetActor");
};
