// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable/InteractableInterface.h"
#include "AbilitySystemInterface.h"
#include "InteractableAccioTarget.generated.h"

class UStaticMeshComponent;
class UAbilitySystemComponent;

UCLASS()
class HOGWARTSLEGACYCLONE_API AInteractableAccioTarget : public AActor, public IInteractableInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:	
	AInteractableAccioTarget();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

protected:
	virtual void BeginPlay() override;

public:	
	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual void Interact_Implementation(AActor* Interactor) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
};
