// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/Enemy/DA_EnemyConfigBase.h"
#include "DA_DementorConfig.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UDA_DementorConfig : public UDA_EnemyConfigBase
{
	GENERATED_BODY()
	
	
	// 비행
	UPROPERTY(EditAnywhere, Category = "Flight")
	float HoverHeight = 150.f;
};
