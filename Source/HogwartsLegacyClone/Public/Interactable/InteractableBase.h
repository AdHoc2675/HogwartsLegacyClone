// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable/InteractableInterface.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "InteractableBase.generated.h"

class USceneComponent;
class UAbilitySystemComponent;

/**
 * 상호작용 가능한 오브젝트들의 공통 베이스
 * - IInteractableInterface 공통 구현
 * - IAbilitySystemInterface 공통 구현
 * - ASC 보유
 * - BeginPlay 시 Team.Object 자동 부여
 * - 자식별 초기 태그 세팅 훅 제공
 * - 자식별 실제 상호작용 처리 훅 제공
 */
UCLASS(Abstract)
class HOGWARTSLEGACYCLONE_API AInteractableBase
	: public AActor
	  , public IInteractableInterface
	  , public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AInteractableBase();

public:
	/* ==============================
	   IAbilitySystemInterface
	================================ */
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
	{
		return AbilitySystemComponent;
	}

protected:
	virtual void BeginPlay() override;

public:
	/* ==============================
	   IInteractableInterface
	================================ */
	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual void Interact_Implementation(AActor* Interactor) override;
	
	virtual bool CanReceiveInteractionTag_Implementation(AActor* Interactor, FGameplayTag InteractionTag) override;
	virtual void ReceiveInteractionTag_Implementation(AActor* Interactor, FGameplayTag InteractionTag) override;

protected:
	/** 자식이 실제 상호작용 결과를 구현하는 곳 */
	virtual void HandleInteract(AActor* Interactor);

	/** 자식이 실제 태그 기반 상호작용 결과를 구현하는 곳 */
	virtual void HandleReceiveInteractionTag(AActor* Interactor, FGameplayTag InteractionTag);

	/** BP/C++에서 설정한 기본 태그를 ASC에 적용 */
	virtual void ApplyDefaultOwnedTags();

	
	/** 공통 ASC 유효성 체크 헬퍼 */
	bool HasAbilitySystemComponent() const;
	

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interactable")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interactable|GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** 상속 BP에서 Team.Object, Interactable.* , 초기 상태 태그 등을 넣는 곳 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Interactable|GAS")
	FGameplayTagContainer DefaultOwnedTags;
};
