// Copyright Epic Games, Inc. All Rights Reserved.

#include "HogwartsLegacyCloneGameMode.h"
#include "HogwartsLegacyCloneCharacter.h"
#include "UObject/ConstructorHelpers.h"

AHogwartsLegacyCloneGameMode::AHogwartsLegacyCloneGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
