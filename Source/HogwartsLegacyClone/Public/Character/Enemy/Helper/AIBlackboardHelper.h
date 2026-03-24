// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "AIBlackboardHelper.generated.h"

/**
 * 
 */
class UBlackboardComponent;

UCLASS()
class HOGWARTSLEGACYCLONE_API UAIBlackboardHelper : public UObject
{
	GENERATED_BODY()

public:
	static AActor* GetTargetActor(UBlackboardComponent* BB);
	static void SetTargetActor(UBlackboardComponent* BB, AActor* Actor);
	static void ClearTargetActor(UBlackboardComponent* BB);

	static float GetTargetDistance(UBlackboardComponent* BB);
	static void SetTargetDistance(UBlackboardComponent* BB, float Distance);

	static FName GetAbilityTagName(UBlackboardComponent* BB);
	static void SetAbilityTagName(UBlackboardComponent* BB, FName TagName);

	static bool GetChaseDelay(UBlackboardComponent* BB);
	static void SetChaseDelay(UBlackboardComponent* BB, bool bInDelay);

private:
	static inline const FName KEY_TargetActor{TEXT("TargetActor")};
	static inline const FName KEY_TargetDistance{TEXT("TargetDistance")};
	static inline const FName KEY_AbilityTag{TEXT("AbilityTag")};
	static inline const FName KEY_ChaseDelay{TEXT("ChaseDelay")};
};
