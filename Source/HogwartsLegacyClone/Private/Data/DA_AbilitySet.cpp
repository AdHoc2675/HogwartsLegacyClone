// Fill out your copyright notice in the Description page of Project Settings.


#include "Data/DA_AbilitySet.h"

#include "AbilitySystemComponent.h"
#include "HOGDebugHelper.h"
#include "Abilities/GameplayAbility.h"


void UDA_AbilitySet::GiveAbilities(UAbilitySystemComponent* ASC) const
{
	if (!ASC)
	{
		// Debug::Print(TEXT("[AbilitySet] ASC is null"), FColor::Red);
		// return;
	}

	// Debug::Print(FString::Printf(
	// 	TEXT("[AbilitySet] GiveAbilities Start | ASC=%s | AbilityCount=%d"),
	// 	*GetNameSafe(ASC),
	// 	Abilities.Num()
	// ), FColor::Cyan);

	for (const FHOGAbilitySet& Entry : Abilities)
	{
		if (!Entry.Ability)
		{
			// Debug::Print(TEXT("[AbilitySet] Skipped: Ability is null"), FColor::Yellow);
			// continue;
		}

		FGameplayAbilitySpec Spec(Entry.Ability, Entry.AbilityLevel);

		if (Entry.InputTag.IsValid())
		{
			Spec.DynamicAbilityTags.AddTag(Entry.InputTag);
		}

		const FGameplayAbilitySpecHandle GivenHandle = ASC->GiveAbility(Spec);

		// Debug::Print(FString::Printf(
		// 	TEXT("[AbilitySet] Granted | Ability=%s | InputTag=%s | Level=%d | HandleValid=%d"),
		// 	*GetNameSafe(Entry.Ability),
		// 	*Entry.InputTag.ToString(),
		// 	Entry.AbilityLevel,
		// 	GivenHandle.IsValid() ? 1 : 0
		// ), FColor::Green);
	}	
	
}
