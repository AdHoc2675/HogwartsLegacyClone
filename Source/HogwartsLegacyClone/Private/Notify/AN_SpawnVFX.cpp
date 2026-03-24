// Fill out your copyright notice in the Description page of Project Settings.


#include "Notify/AN_SpawnVFX.h"

#include "Character/Player/PlayerCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"

void UAN_SpawnVFX::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::Notify(MeshComp, Animation);

	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(OwnerActor);
	if (!PlayerCharacter)
	{
		return;
	}

	// 1) 기존 라인트레이스/큐 기반 VFX 소비
	PlayerCharacter->ConsumeAndSpawnQueuedSpellVFX();

	// 2) 이번 프레임에 등록된 Spell Ability CastNotify 소비
	PlayerCharacter->ConsumeCastNotifyAbility();
}

FString UAN_SpawnVFX::GetNotifyName_Implementation() const
{
	return TEXT("SpawnVFX");
}