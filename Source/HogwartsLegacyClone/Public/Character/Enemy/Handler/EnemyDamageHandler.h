// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "EnemyDamageHandler.generated.h"

/**
 * 
 */
class AEnemyCharacterBase;
class UDamageNumberPool;

UCLASS()
class HOGWARTSLEGACYCLONE_API UEnemyDamageHandler : public UObject
{
	GENERATED_BODY()
	
public:
	void Initialize(AEnemyCharacterBase* InOwner);
	void Shutdown();

private:
	// 피격 반응
	UFUNCTION()
	void HandleHitReact(float Damage);
    
	// 데미지 넘버
	UFUNCTION()
	void SpawnDamageNumber(float Damage);

	UPROPERTY()
	TWeakObjectPtr<AEnemyCharacterBase> OwnerEnemy;

	UPROPERTY()
	TWeakObjectPtr<UDamageNumberPool> DamageNumberPool;

	// 데미지 넘버 스택 관리
	double LastDamageNumberTime = 0.0;
	float LastDamageNumberZ = 0.f;
	float DamageNumberSpacing = 30.f;
	
};
