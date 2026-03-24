// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DistanceCalculator.generated.h"

/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UDistanceCalculator : public UObject
{
	GENERATED_BODY()
	
	public:
	static float Calculate(AActor* From, AActor* To);
	
};
