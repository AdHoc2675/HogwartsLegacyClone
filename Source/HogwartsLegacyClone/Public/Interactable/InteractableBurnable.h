// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable/InteractableInterface.h"
#include "AbilitySystemInterface.h"
#include "InteractableBurnable.generated.h"

class UStaticMeshComponent;
class UNiagaraComponent;
class UAbilitySystemComponent;

UCLASS()
class HOGWARTSLEGACYCLONE_API AInteractableBurnable : public AActor, public IInteractableInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AInteractableBurnable();

	// IAbilitySystemInterface 구현
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// IInteractableInterface 구현부
	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	// 불이 붙을 때 실행할 블루프린트 이벤트 (사운드 등 편의용)
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void PlayIgniteEffects();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraComponent> FireVFXComp;

	// 태그 관리를 위한 ASC
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

};
