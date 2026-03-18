#include "Interactable/InteractableChest.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/HOG_GameplayTags.h"

AInteractableChest::AInteractableChest()
{
	PrimaryActorTick.bCanEverTick = false;

	BaseMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(SceneRoot);
}

bool AInteractableChest::CanInteract_Implementation(AActor* Interactor)
{
	return AbilitySystemComponent
		&& AbilitySystemComponent->HasMatchingGameplayTag(HOGGameplayTags::Interactable_Chest_Closed);
}

void AInteractableChest::HandleInteract(AActor* Interactor)
{
	Super::HandleInteract(Interactor);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(HOGGameplayTags::Interactable_Chest_Closed);
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Interactable_Chest_Opened);
	}

	PlayOpenAnimation();

	if (BaseMesh && OpenMontage)
	{
		if (UAnimInstance* AnimInstance = BaseMesh->GetAnimInstance())
		{
			AnimInstance->Montage_Play(OpenMontage, 1.0f);
		}
	}

	// TODO: 아이템 지급, 사운드 재생 등 추가 효과
}