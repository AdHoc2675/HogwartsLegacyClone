#include "Notify/AN_BasicAttackFire.h"

#include "Character/Player/PlayerCharacterBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

#include "GAS/Abilities/Spell/BasicAttack/GA_Spell_BasicAttack.h"
#include "GameFramework/HOG_PlayerState.h"

#include "HOGDebugHelper.h"

void UAN_BasicAttackFire::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation
)
{
	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();

	if (!OwnerActor)
	{
		return;
	}


	ACharacter* Character = Cast<ACharacter>(OwnerActor);
	if (!Character)
	{
		return;
	}

	AHOG_PlayerState* PS = Character->GetPlayerState<AHOG_PlayerState>();
	if (!PS)
	{
		return;
	}

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}


	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		UGameplayAbility* Ability = Spec.GetPrimaryInstance();

		if (!Ability)
		{
			continue;
		}

		UGA_Spell_BasicAttack* BasicAttackAbility =
			Cast<UGA_Spell_BasicAttack>(Ability);

		if (BasicAttackAbility)
		{
			BasicAttackAbility->SpawnBasicAttackActor();

			return;
		}
	}
}
