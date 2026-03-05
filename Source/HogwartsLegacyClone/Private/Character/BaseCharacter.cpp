// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BaseCharacter.h"

#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->bReceivesDecals = false;
	}
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ABaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	//나중에 GAS용
}

void ABaseCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	//나중에 GAS용
}

void ABaseCharacter::SetTeamTag(FGameplayTag NewTeamTag)
{
	TeamTag = NewTeamTag;
}

bool ABaseCharacter::HasTeamTag(FGameplayTag QueryTag) const
{
	return TeamTag.IsValid() && TeamTag.MatchesTag(QueryTag);
}

void ABaseCharacter::Die()
{
	HandleDeath();
}

void ABaseCharacter::HandleDeath_Implementation()
{
	if (bIsDead)return;
	bIsDead = true;

	//일단 정지처리만 구현

	UCharacterMovementComponent* MoveComp = Cast<UCharacterMovementComponent>(GetCharacterMovement());
	if (MoveComp)
	{
		MoveComp->DisableMovement();
	}
}
