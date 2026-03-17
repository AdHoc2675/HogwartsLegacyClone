#pragma once

#include "CoreMinimal.h"
#include "Interactable/InteractableBase.h"
#include "InteractableChest.generated.h"

class USkeletalMeshComponent;
class UAnimMontage;

UCLASS()
class HOGWARTSLEGACYCLONE_API AInteractableChest : public AInteractableBase
{
	GENERATED_BODY()
	
public:
	AInteractableChest();

public:
	virtual bool CanInteract_Implementation(AActor* Interactor) override;

protected:
	virtual void HandleInteract(AActor* Interactor) override;

public:
	// 타겟 애니메이션 연출
	UFUNCTION(BlueprintImplementableEvent, Category="Interaction")
	void PlayOpenAnimation();

protected:
	// 상자 메쉬
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh")
	TObjectPtr<USkeletalMeshComponent> BaseMesh;

	// 상자가 열릴 때 재생할 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Animation")
	TObjectPtr<UAnimMontage> OpenMontage;
};