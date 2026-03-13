#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "DT_AttributeRow.generated.h"


/**
 * Attribute 초기값 DataTable Row
 */
USTRUCT(BlueprintType)
struct HOGWARTSLEGACYCLONE_API FDT_AttributeRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	/* 유닛 식별 태그 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attribute")
	FGameplayTag UnitTag;
	
	/* 현재 체력 초기값 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attribute")
	float Health = 100.f;

	/* 최대 체력 초기값 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attribute")
	float MaxHealth = 100.f;

	/* 공격력 초기값 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attribute")
	float AttackPower = 10.f;
};