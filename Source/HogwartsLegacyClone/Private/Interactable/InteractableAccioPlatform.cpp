// Fill out your copyright notice in the Description page of Project Settings.

#include "Interactable/InteractableAccioPlatform.h"
#include "Components/StaticMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "Core/HOG_GameplayTags.h"

// Sets default values
AInteractableAccioPlatform::AInteractableAccioPlatform()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;
	
	// 물리 활성화 및 마찰 설정
	BaseMesh->SetSimulatePhysics(true);
	BaseMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

// Called when the game starts or when spawned
void AInteractableAccioPlatform::BeginPlay()
{
	Super::BeginPlay();

	if (BaseMesh)
	{
		FBodyInstance* BodyInst = BaseMesh->GetBodyInstance();
		if (BodyInst)
		{
			// 회전 및 Z축 고정 (기울어지거나 들썩이지 않고 수평으로만 움직임)
			BodyInst->bLockXRotation = true;
			BodyInst->bLockYRotation = true;
			BodyInst->bLockZRotation = true;
			BodyInst->bLockZTranslation = true; // Z축 승강 제한
			
			BodyInst->SetDOFLock(EDOFMode::SixDOF);

			// 플레이어가 타도 밀리지 않도록 질량 크게 설정
			BaseMesh->SetMassOverrideInKg(NAME_None, 10000.0f, true);
			BaseMesh->SetLinearDamping(2.0f); // 미끄러짐 방지 제한
		}
	}

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Team_Object);
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Interactable_AccioPlatform); 
	}
}

bool AInteractableAccioPlatform::CanInteract_Implementation(AActor* Interactor)
{
    return true;
}

void AInteractableAccioPlatform::Interact_Implementation(AActor* Interactor)
{
    // C++ 인터페이스 규칙 검사
    if (!IInteractableInterface::Execute_CanInteract(this, Interactor)) return;
    // 발판 자체 상호작용은 특별한 처리가 필요하지 않고 Accio 로직에서 직접 제어
}

