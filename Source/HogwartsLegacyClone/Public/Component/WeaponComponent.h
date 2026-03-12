// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "WeaponComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HOGWARTSLEGACYCLONE_API UWeaponComponent : public UStaticMeshComponent
{
	GENERATED_BODY()
	
	public:
	UPROPERTY(EditAnywhere, Category="Socket")
	FName StartSocketName = TEXT("WeaponStart");
	
	UPROPERTY(EditAnywhere, Category="Socket")
	FName EndSocketName = TEXT("WeaponEnd");
	
};
