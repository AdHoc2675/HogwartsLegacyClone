#pragma once

#include "CoreMinimal.h"
#include "Interactable/InteractableBase.h"
#include "InteractableLevitatable.generated.h"

class UStaticMeshComponent;
class UAbilitySystemComponent;
class UNiagaraComponent;

UCLASS()
class HOGWARTSLEGACYCLONE_API AInteractableLevitatable : public AInteractableBase
{
	GENERATED_BODY()
	
public:
	AInteractableLevitatable();

protected:
	virtual void BeginPlay() override;

public:
	virtual bool CanInteract_Implementation(AActor* Interactor) override;

protected:
	virtual void HandleInteract(AActor* Interactor) override;

public:
	UFUNCTION(BlueprintCallable, Category="Interaction")
	void StopLevitation();

	UFUNCTION(BlueprintImplementableEvent, Category="Interaction")
	void OnLevitated();
	
	UFUNCTION(BlueprintImplementableEvent, Category="Interaction")
	void OnDropped();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="VFX")
	TObjectPtr<UNiagaraComponent> MagicAuraVFXComp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Levitation")
	float LevitateForce = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HOG|Levitate")
	bool bIsPlatformMode = true;
};