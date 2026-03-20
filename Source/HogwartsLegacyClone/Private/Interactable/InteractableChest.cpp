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

	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Green, TEXT("상호작용 진입 성공"));


	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(HOGGameplayTags::Interactable_Chest_Closed);
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Interactable_Chest_Opened);
	}


	if (BaseMesh && OpenMontage)
	{
		if (UAnimInstance* AnimInstance = BaseMesh->GetAnimInstance())
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, TEXT("AnimInstance 찾음! 몽타주 재생 시도"));
			AnimInstance->Montage_Play(OpenMontage, 1.0f);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("AnimInstance가 없습니다! AnimBP 세팅 필요"));
		}
	}

	// TODO: 아이템 지급, 사운드 재생 등 추가 효과
}