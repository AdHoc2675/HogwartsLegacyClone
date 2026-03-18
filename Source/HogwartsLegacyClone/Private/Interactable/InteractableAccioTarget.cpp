#include "Interactable/InteractableAccioTarget.h"

#include "Components/StaticMeshComponent.h"

AInteractableAccioTarget::AInteractableAccioTarget()
{
	PrimaryActorTick.bCanEverTick = false;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(SceneRoot);

	// 타겟은 고정 오브젝트
	BaseMesh->SetSimulatePhysics(false);
}

void AInteractableAccioTarget::HandleInteract(AActor* Interactor)
{
	Super::HandleInteract(Interactor);

	// 고정 타겟 자체는 별도 상호작용 처리 없음
	// Accio 스펠 로직 또는 외부 시스템에서 직접 사용
}