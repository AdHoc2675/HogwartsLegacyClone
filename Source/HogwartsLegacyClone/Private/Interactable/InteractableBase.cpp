#include "Interactable/InteractableBase.h"

#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Core/HOG_GameplayTags.h"

AInteractableBase::AInteractableBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
}

void AInteractableBase::BeginPlay()
{
	Super::BeginPlay();

	ApplyDefaultOwnedTags();
}

bool AInteractableBase::CanInteract_Implementation(AActor* Interactor)
{
	// 기본값은 true
	// 상태 기반 제약이 필요한 자식은 override 해서 사용
	return true;
}

void AInteractableBase::Interact_Implementation(AActor* Interactor)
{
	// 공통 가드
	if (!IInteractableInterface::Execute_CanInteract(this, Interactor))
	{
		return;
	}

	// 실제 효과는 자식이 구현
	HandleInteract(Interactor);
}

bool AInteractableBase::CanReceiveInteractionTag_Implementation(AActor* Interactor, FGameplayTag InteractionTag)
{
	// 기본값은 true
	// 태그 기반 제약이 필요한 자식은 override 해서 사용
	return InteractionTag.IsValid();
}

void AInteractableBase::ReceiveInteractionTag_Implementation(AActor* Interactor, FGameplayTag InteractionTag)
{
	// 공통 가드
	if (!IInteractableInterface::Execute_CanReceiveInteractionTag(this, Interactor, InteractionTag))
	{
		return;
	}

	// 실제 효과는 자식이 구현
	HandleReceiveInteractionTag(Interactor, InteractionTag);
}

void AInteractableBase::HandleReceiveInteractionTag(AActor* Interactor, FGameplayTag InteractionTag)
{
	// Base 기본 구현은 비워둠
	// 자식에서 override 하여 실제 태그 기반 상호작용 처리
}

void AInteractableBase::HandleInteract(AActor* Interactor)
{
	// Base 기본 구현은 비워둠
	// 자식에서 override 하여 실제 상호작용 처리
}

void AInteractableBase::ApplyDefaultOwnedTags()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	for (const FGameplayTag& Tag : DefaultOwnedTags)
	{
		if (Tag.IsValid())
		{
			AbilitySystemComponent->AddLooseGameplayTag(Tag);
		}
	}
}

bool AInteractableBase::HasAbilitySystemComponent() const
{
	return AbilitySystemComponent != nullptr;
}
