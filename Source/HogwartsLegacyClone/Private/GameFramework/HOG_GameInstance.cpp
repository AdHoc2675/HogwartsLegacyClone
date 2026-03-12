// Fill out your copyright notice in the Description page of Project Settings.


#include "GameFramework/HOG_GameInstance.h"

#include "Data/DA_SpellDefinition.h"
#include "HOGDebugHelper.h"

void UHOG_GameInstance::Init()
{
	Super::Init();
	
	BuildSpellRegistry();
}

void UHOG_GameInstance::BuildSpellRegistry()
{
	SpellRegistry.Reset();

	int32 AddedCount = 0;

	for (UDA_SpellDefinition* Def : SpellDefinitions)
	{
		// if (!IsValid(Def))
		// {
		// 	Debug::Print(TEXT("[SpellRegistry] Null Definition in SpellDefinitions array."), FColor::Red);
		// 	continue;
		// }
		//
		// if (!Def->IsValidDefinition())
		// {
		// 	Debug::Print(FString::Printf(TEXT("[SpellRegistry] Invalid SpellDefinition: %s (SpellID invalid)"), *GetNameSafe(Def)), FColor::Red);
		// 	continue;
		// }

		const FGameplayTag ID = Def->SpellID;

		if (SpellRegistry.Contains(ID))
		{
			UDA_SpellDefinition* Existing = SpellRegistry.FindRef(ID);
			// Debug::Print(FString::Printf(
			// 	TEXT("[SpellRegistry] Duplicate SpellID: %s | Existing=%s | New=%s"),
			// 	*ID.ToString(),
			// 	*GetNameSafe(Existing),
			// 	*GetNameSafe(Def)
			// ), FColor::Yellow);

			// 중복이면 기존 유지(정책). 필요하면 New로 덮어쓰게 바꿔도 됨.
			continue;
		}

		SpellRegistry.Add(ID, Def);
		AddedCount++;
	}

	// Debug::Print(FString::Printf(TEXT("[SpellRegistry] Built. Count=%d"), AddedCount), FColor::Green);
}

UDA_SpellDefinition* UHOG_GameInstance::GetSpellDefinition(FGameplayTag SpellID) const
{
	if (!SpellID.IsValid())
	{
		return nullptr;
	}

	if (const TObjectPtr<UDA_SpellDefinition>* Found = SpellRegistry.Find(SpellID))
	{
		return Found->Get();
	}

	return nullptr;
}
