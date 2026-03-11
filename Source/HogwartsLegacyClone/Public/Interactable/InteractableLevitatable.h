// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactable/InteractableInterface.h"
#include "AbilitySystemInterface.h"
#include "InteractableLevitatable.generated.h"

class UStaticMeshComponent;
class UAbilitySystemComponent;
class UNiagaraComponent;

UCLASS()
class HOGWARTSLEGACYCLONE_API AInteractableLevitatable : public AActor, public IInteractableInterface, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AInteractableLevitatable();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// IInteractableInterface 구현부
	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual void Interact_Implementation(AActor* Interactor) override;

	// 레비오사 효과 종료 (내려놓기)
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void StopLevitation();

	// 시각 효과/사운드용 블루프린트 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnLevitated();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void OnDropped();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraComponent> MagicAuraVFXComp; // 떠오를 때 밑에서 빛나는 아우라 등

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	// 이 물체가 뜰 높이의 힘 (Z방향 속도)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Levitation")
	float LevitateForce = 300.f;
};
