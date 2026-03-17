#pragma once

#include "CoreMinimal.h"
#include "Interactable/InteractableBase.h"
#include "InteractableAccioPlatform.generated.h"

class UStaticMeshComponent;

UCLASS()
class HOGWARTSLEGACYCLONE_API AInteractableAccioPlatform : public AInteractableBase
{
	GENERATED_BODY()
	
public:
	AInteractableAccioPlatform();

protected:
	virtual void BeginPlay() override;

protected:
	/** 실제 상호작용 결과 처리 */
	virtual void HandleInteract(AActor* Interactor) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh")
	TObjectPtr<UStaticMeshComponent> BaseMesh;
};