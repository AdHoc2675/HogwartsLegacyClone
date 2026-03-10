// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactable/InteractableChest.h"
#include "AbilitySystemComponent.h"
#include "Core/HOG_GameplayTags.h"
#include "Animation/AnimInstance.h"

// Sets default values
AInteractableChest::AInteractableChest()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	BaseMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BaseMesh"));
	RootComponent = BaseMesh;
}

// Called when the game starts or when spawned
void AInteractableChest::BeginPlay()
{
	Super::BeginPlay();
	
	if (AbilitySystemComponent)
	{
		// 오브젝트 소속 태그 부여
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Team_Object);

		// 초기 상태를 '닫힘(Closed)'으로 설정
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Interactable_Chest_Closed);
	}
}

bool AInteractableChest::CanInteract_Implementation(AActor* Interactor)
{
	// ASC가 타겟에 있다면, 타겟이 현재 "닫힌" 상태일 때만 상호작용 가능
	return AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(HOGGameplayTags::Interactable_Chest_Closed);
}

void AInteractableChest::Interact_Implementation(AActor* Interactor)
{
	// 1. CanInteract 로직을 통해 상태 확인 (태그 기반)
	if (!CanInteract(Interactor))
	{
		return;
	}

	// 2. 상태 전환: 닫힘 태그 제거, 열림 태그 추가
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(HOGGameplayTags::Interactable_Chest_Closed);
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Interactable_Chest_Opened);
	}

	// 3. Blueprint 이벤트 호출 (필요한 경우)
	PlayOpenAnimation();

	// 4. 몽타주 재생
	if (BaseMesh && OpenMontage)
	{
		UAnimInstance* AnimInstance = BaseMesh->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Play(OpenMontage, 1.0f);
		}
	}

	//TODO: 아이템 지급, 사운드 재생 등 추가적인 상호작용 효과 구현 
}