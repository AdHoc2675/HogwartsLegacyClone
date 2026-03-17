#pragma once

#include "CoreMinimal.h"
#include "Interactable/InteractableBase.h"
#include "InteractableBurnable.generated.h"

class UStaticMeshComponent;
class UNiagaraComponent;

UCLASS()
class HOGWARTSLEGACYCLONE_API AInteractableBurnable : public AInteractableBase
{
	GENERATED_BODY()
	
public:
	AInteractableBurnable();

public:
	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual bool CanReceiveInteractionTag_Implementation(AActor* Interactor, FGameplayTag InteractionTag) override;

protected:
	virtual void HandleInteract(AActor* Interactor) override;
	virtual void HandleReceiveInteractionTag(AActor* Interactor, FGameplayTag InteractionTag) override;

public:
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction")
	void PlayIgniteEffects();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mesh")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraComponent> FireVFXComp;
};