#include "Interactable/InteractableAccioPlatform.h"

#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/BodyInstance.h"

AInteractableAccioPlatform::AInteractableAccioPlatform()
{
	PrimaryActorTick.bCanEverTick = false;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(SceneRoot);

	// 물리 활성화 및 충돌 설정
	BaseMesh->SetSimulatePhysics(true);
	BaseMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
}

void AInteractableAccioPlatform::BeginPlay()
{
	Super::BeginPlay();

	if (BaseMesh)
	{
		FBodyInstance* BodyInst = BaseMesh->GetBodyInstance();
		if (BodyInst)
		{
			// 회전 고정
			BodyInst->bLockXRotation = true;
			BodyInst->bLockYRotation = true;
			BodyInst->bLockZRotation = true;

			// Z축만 유지하고 흔들림 방지
			BodyInst->bLockZTranslation = true;

			BodyInst->SetDOFLock(EDOFMode::SixDOF);

			// 플레이어가 올라가도 쉽게 밀리지 않도록 무게/감쇠 조정
			BaseMesh->SetMassOverrideInKg(NAME_None, 10000.0f, true);
			BaseMesh->SetLinearDamping(2.0f);
		}
	}
}

void AInteractableAccioPlatform::HandleInteract(AActor* Interactor)
{
	Super::HandleInteract(Interactor);

	// 발판 자체는 별도 상호작용 처리 없음
	// Accio 스펠 로직 쪽에서 직접 제어
}