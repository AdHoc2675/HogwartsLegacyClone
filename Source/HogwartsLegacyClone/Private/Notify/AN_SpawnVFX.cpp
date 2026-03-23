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

	PlayerCharacter->ConsumeAndSpawnQueuedSpellVFX();
}

FString UAN_SpawnVFX::GetNotifyName_Implementation() const
{
	return TEXT("SpawnVFX");
}