#include "Interactable/InteractableLevitatable.h"

#include "Components/StaticMeshComponent.h"
#include "AbilitySystemComponent.h"
#include "NiagaraComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Core/HOG_GameplayTags.h"
#include "HOGDebugHelper.h"

AInteractableLevitatable::AInteractableLevitatable()
{
	PrimaryActorTick.bCanEverTick = false;

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(SceneRoot);

	BaseMesh->SetSimulatePhysics(true);
	BaseMesh->SetCollisionProfileName(TEXT("PhysicsActor"));

	MagicAuraVFXComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("MagicAuraVFXComp"));
	MagicAuraVFXComp->SetupAttachment(SceneRoot);
	MagicAuraVFXComp->SetAutoActivate(false);
}

void AInteractableLevitatable::BeginPlay()
{
	Super::BeginPlay();

	if (bIsPlatformMode && BaseMesh)
	{
		if (FBodyInstance* BodyInst = BaseMesh->GetBodyInstance())
		{
			BodyInst->bLockXRotation = true;
			BodyInst->bLockYRotation = true;
			BodyInst->bLockZRotation = true;

			BodyInst->bLockXTranslation = true;
			BodyInst->bLockYTranslation = true;

			BodyInst->SetDOFLock(EDOFMode::SixDOF);

			BaseMesh->SetMassOverrideInKg(NAME_None, 5000.0f, true);
		}
	}
}

bool AInteractableLevitatable::CanInteract_Implementation(AActor* Interactor)
{
	return AbilitySystemComponent
		&& AbilitySystemComponent->HasMatchingGameplayTag(HOGGameplayTags::Interactable_Levitatable_Grounded);
}

void AInteractableLevitatable::HandleInteract(AActor* Interactor)
{
	Super::HandleInteract(Interactor);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(HOGGameplayTags::Interactable_Levitatable_Grounded);
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::State_Spell_Leviosa_Levitated);
	}

	//Debug::Print(TEXT("[Levitatable] 마법 적중, 부유 시작"), FColor::Cyan);

	if (MagicAuraVFXComp)
	{
		MagicAuraVFXComp->Activate(true);
	}

	if (BaseMesh)
	{
		BaseMesh->SetEnableGravity(false);
		BaseMesh->SetPhysicsLinearVelocity(FVector(0.f, 0.f, LevitateForce));
		BaseMesh->SetLinearDamping(5.0f);
		BaseMesh->SetAngularDamping(2.0f);
	}

	OnLevitated();
}

void AInteractableLevitatable::StopLevitation()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(HOGGameplayTags::State_Spell_Leviosa_Levitated);
		AbilitySystemComponent->AddLooseGameplayTag(HOGGameplayTags::Interactable_Levitatable_Grounded);
	}

	Debug::Print(TEXT("[Levitatable] 마법 해제, 부유 종료"), FColor::Red);

	if (MagicAuraVFXComp)
	{
		MagicAuraVFXComp->Deactivate();
	}

	if (BaseMesh)
	{
		BaseMesh->SetEnableGravity(true);
		BaseMesh->SetLinearDamping(0.01f);
		BaseMesh->SetAngularDamping(0.0f);
	}

	OnDropped();
}