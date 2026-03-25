// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotify_WeaponTrace.generated.h"

class UWeaponComponent;
class AEnemyCharacterBase;
/**
 * 
 */
UCLASS()
class HOGWARTSLEGACYCLONE_API UAnimNotify_WeaponTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp,
	                         UAnimSequenceBase* Animation, float TotalDuration) override;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp,
	                        UAnimSequenceBase* Animation, float FrameDeltaTime) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp,
	                       UAnimSequenceBase* Animation) override;


	UPROPERTY(EditAnywhere)
	float TraceRadius = 70.f;

private:
	void DoTrace(USkeletalMeshComponent* MeshComp);

	TWeakObjectPtr<AEnemyCharacterBase> Enemy;
	TWeakObjectPtr<UWeaponComponent> Weapon;
	TArray<AActor*> HitActors;
};
