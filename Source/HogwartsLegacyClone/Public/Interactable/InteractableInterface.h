// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "InteractableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class HOGWARTSLEGACYCLONE_API IInteractableInterface
{
	GENERATED_BODY()

public:
	// 상호작용 가능 여부 확인
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AActor* Interactor);

	// 실제 상호작용 실행 (ex: 상자 열기 애니메이션, 아이템 지급 등)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(AActor* Interactor);
	
	// 상호작용 태그 수신 가능 여부 확인
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanReceiveInteractionTag(AActor* Interactor, FGameplayTag InteractionTag);

	// 상호작용 태그 수신 처리
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void ReceiveInteractionTag(AActor* Interactor, FGameplayTag InteractionTag);
};
