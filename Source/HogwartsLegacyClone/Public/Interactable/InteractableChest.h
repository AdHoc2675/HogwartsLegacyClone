// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractableInterface.h"
#include "AbilitySystemInterface.h"
#include "InteractableChest.generated.h"

class USkeletalMeshComponent;
class UAbilitySystemComponent;

UCLASS()
class HOGWARTSLEGACYCLONE_API AInteractableChest : public AActor, public IInteractableInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AInteractableChest();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// IInteractableInterface 구현부
	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	// 상자 메쉬 (뚜껑 애니메이션 등을 위해 분리 권장)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<USkeletalMeshComponent> BaseMesh;

	// 태그 관리를 위한 기초 ASC 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	// 타겟 애니메이션 연출
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void PlayOpenAnimation();

	// 상자가 열릴 때 재생할 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> OpenMontage;
};
