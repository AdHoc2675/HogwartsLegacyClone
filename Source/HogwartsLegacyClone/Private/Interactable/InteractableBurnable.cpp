#include "Interactable/InteractableBurnable.h"

#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"
#include "AbilitySystemComponent.h"
#include "Core/HOG_GameplayTags.h"
#include "HOGDebugHelper.h"

AInteractableBurnable::AInteractableBurnable()
{
	PrimaryActorTick.bCanEverTick = false;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(SceneRoot);

	FireVFXComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FireVFXComp"));
	FireVFXComp->SetupAttachment(SceneRoot);
	FireVFXComp->SetAutoActivate(false);
}

bool AInteractableBurnable::CanInteract_Implementation(AActor* Interactor)
{
	return AbilitySystemComponent
		&& AbilitySystemComponent->HasMatchingGameplayTag(HOGGameplayTags::Interactable_Burnable_Unlit);
}

void AInteractableBurnable::HandleInteract(AActor* Interactor)
{
	Super::HandleInteract(Interactor);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(HOGGameplayTags::Interactable_Burnable_Unlit);
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Interactable_Burnable_Lit);
	}

	const FString InteractorName = Interactor ? Interactor->GetName() : TEXT("Unknown");
	Debug::Print(
		FString::Printf(TEXT("[Burnable] %s에 마법이 적중하여 점화 (By %s)"), *GetName(), *InteractorName),
		FColor::Orange
	);

	if (FireVFXComp)
	{
		FireVFXComp->Activate(true);
	}

	PlayIgniteEffects();
}