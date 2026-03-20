// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "DA_InputConfig.generated.h"

class UInputMappingContext;
class UInputAction;

/**
 * Input 설정 DataAsset
 * - PlayerController가 읽어서 EnhancedInput 바인딩을 구성한다.
 */

UCLASS()
class HOGWARTSLEGACYCLONE_API UDA_InputConfig : public UDataAsset
{
	GENERATED_BODY()
	
public:

	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputMappingContext* DefaultMappingContext;

	/** 이동 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputAction* IA_Move;

	/** 시점 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputAction* IA_Look;

	/** 점프 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	UInputAction* IA_Jump;

	/** 상호작용 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	UInputAction* IA_Interact;

	/** Ability 입력 (Tag 기반) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
	TMap<FGameplayTag, UInputAction*> AbilityInputActions;
};
