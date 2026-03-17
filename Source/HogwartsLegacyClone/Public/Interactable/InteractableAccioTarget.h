#pragma once

#include "CoreMinimal.h"
#include "Interactable/InteractableBase.h"
#include "InteractableAccioTarget.generated.h"

class UStaticMeshComponent;

UCLASS()
class HOGWARTSLEGACYCLONE_API AInteractableAccioTarget : public AInteractableBase
{
	GENERATED_BODY()
	
public:
	AInteractableAccioTarget();

protected:
	/** 실제 상호작용 결과 처리 */
	virtual void HandleInteract(AActor* Interactor) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh")
	TObjectPtr<UStaticMeshComponent> BaseMesh;
};