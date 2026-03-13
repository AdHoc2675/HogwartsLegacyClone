// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactable/InteractableAccioTarget.h"
#include "Components/StaticMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "Core/HOG_GameplayTags.h"

AInteractableAccioTarget::AInteractableAccioTarget()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;
    
    // 타겟은 고정되어 움직이지 않음
	BaseMesh->SetSimulatePhysics(false);
	
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

void AInteractableAccioTarget::BeginPlay()
{
	Super::BeginPlay();
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Team_Object);
	}
}

bool AInteractableAccioTarget::CanInteract_Implementation(AActor* Interactor)
{
	return true;
}

void AInteractableAccioTarget::Interact_Implementation(AActor* Interactor)
{
	if (!IInteractableInterface::Execute_CanInteract(this, Interactor)) return;
    // 고정 타겟
}

