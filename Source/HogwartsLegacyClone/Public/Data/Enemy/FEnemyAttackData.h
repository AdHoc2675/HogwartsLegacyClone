#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "FEnemyAttackData.generated.h"

class UAnimMontage;
USTRUCT(BlueprintType)
struct FEnemyAttackData
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	FGameplayTag AbilityTag;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UAnimMontage> AnimMontage = nullptr;
	
	UPROPERTY(EditAnywhere)
	float Damage = 10.f;
	
	UPROPERTY(EditAnywhere)
	float MinRange = 0.f;
	
	UPROPERTY(EditAnywhere)
	float MaxRange = 200.f;
};
